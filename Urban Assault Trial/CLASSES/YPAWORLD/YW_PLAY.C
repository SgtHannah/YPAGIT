/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_play.c,v $
**  $Revision: 38.7 $
**  $Date: 1998/01/06 16:25:27 $
**  $Locker:  $
**  $Author: floh $
**
**  Scene-Player- (und Cutter) Funktionen für YPA.
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
#include "engine/engine.h"
#include "visualstuff/ov_engine.h"
#include "bitmap/displayclass.h"
#include "ypa/ypaworldclass.h"
#include "ypa/guimap.h"
#include "ypa/guifinder.h"

#include "thirdparty/eulerangles.h"

#include "yw_protos.h"

_extern_use_nucleus
_extern_use_audio_engine
_extern_use_tform_engine
_extern_use_ov_engine
_extern_use_input_engine

extern struct YPAStatusReq SR;
extern struct YPAMapReq MR;
extern struct YPAFinder FR;

/*-----------------------------------------------------------------*/
/*
**  FUNCTION
**      Euler-2-Matrix-Funktion.
**      (C) Ken Shoemake, veröffentlicht in Graphics Gems IV.
**
**  CHANGED
**      13-Jan-97   floh
*/
/* Construct matrix from Euler angles (in radians). */
void Eul_ToHMatrix(EulerAngles ea, HMatrix M)
{
    double ti, tj, th, ci, cj, ch, si, sj, sh, cc, cs, sc, ss;
    int i,j,k,h,n,s,f;
    EulGetOrd(ea.w,i,j,k,h,n,s,f);
    if (f==EulFrmR) {float t = ea.x; ea.x = ea.z; ea.z = t;}
    if (n==EulParOdd) {ea.x = -ea.x; ea.y = -ea.y; ea.z = -ea.z;}
    ti = ea.x;    tj = ea.y;    th = ea.z;
    ci = cos(ti); cj = cos(tj); ch = cos(th);
    si = sin(ti); sj = sin(tj); sh = sin(th);
    cc = ci*ch; cs = ci*sh; sc = si*ch; ss = si*sh;
    if (s==EulRepYes) {
        M[i][i] = cj;     M[i][j] =  sj*si;    M[i][k] =  sj*ci;
        M[j][i] = sj*sh;  M[j][j] = -cj*ss+cc; M[j][k] = -cj*cs-sc;
        M[k][i] = -sj*ch; M[k][j] =  cj*sc+cs; M[k][k] =  cj*cc-ss;
    } else {
        M[i][i] = cj*ch; M[i][j] = sj*sc-cs; M[i][k] = sj*cc+ss;
        M[j][i] = cj*sh; M[j][j] = sj*ss+cc; M[j][k] = sj*cs-sc;
        M[k][i] = -sj;   M[k][j] = cj*si;    M[k][k] = cj*ci;
    }
    M[W][X]=M[W][Y]=M[W][Z]=M[X][W]=M[Y][W]=M[Z][W]=0.0; M[W][W]=1.0;
}

/*-----------------------------------------------------------------*/
void yw_RCRotGlobalY(struct flt_m3x3 *m, FLOAT rot)
/*
**  FUNCTION
**      Rotiert Matrix um globale Y-Achse.
**      Siehe AF's diverse Rotations-Routinen in den
**      Bact-Classes.
**
**  CHANGED
**      13-Jul-96   floh    created
*/
{
    FLOAT sa,ca;
    struct flt_m3x3 rm,nm;

    ca = cos(rot);
    sa = sin(-rot);

    rm.m11 = ca;  rm.m12 = 0.0; rm.m13 = sa;
    rm.m21 = 0.0; rm.m22 = 1.0; rm.m23 = 0.0;
    rm.m31 = -sa; rm.m32 = 0.0; rm.m33 = ca;

    nc_m_mul_m(m, &rm, &nm);
    *m = nm;
}

/*-----------------------------------------------------------------*/
void yw_RCRotLocalX(struct flt_m3x3 *m, FLOAT rot)
/*
**  FUNCTION
**      Rotiert Matrix um lokales X.
**
**  CHANGED
**      13-Jul-96   floh    created
*/
{
    struct flt_m3x3 nm, rm;
    FLOAT sa = sin(rot);
    FLOAT ca = cos(rot);

    rm.m11 = 1.0; rm.m12 = 0.0; rm.m13 = 0.0;
    rm.m21 = 0.0; rm.m22 = ca;  rm.m23 = sa;
    rm.m31 = 0.0; rm.m32 = -sa; rm.m33 = ca;

    nc_m_mul_m(&rm, m, &nm);
    *m = nm;
}

/*-----------------------------------------------------------------*/
void yw_RCDecodeAudioInfo(struct YPASeqData *s)
/*
**  FUNCTION
**      Runlength-Decoder für Audio-Information.
**
**  INPUTS
**      s->buf_audcod = Input-Buffer (kodiert)
**      s->act_audcod = Anzahl Bytes im Input-Buffer
**
**  RESULTS
**      s->buf_audinf = Output-Buffer (dekodiert)
**
**  CHANGED
**      09-Jul-96   floh    created
*/
{
    LONG count;
    UBYTE code;

    UBYTE *inbuf  = s->buf_audcod;
    UBYTE *outbuf = (UBYTE *) s->buf_audinf;

    count = 0;
    while (inbuf < (s->buf_audcod+s->act_audcod)) {
        code = *inbuf++;    // Steuerbyte lesen
        if (code < 0x80) {  // Steuerbyte+1 Bytes übertragen
            memcpy(outbuf,inbuf,++code);
            outbuf += code;
            inbuf  += code;
        } else if (code > 0x80) {   // Runlegth-Code
            ULONG i;
            code = ~code+2;
            for (i=0; i<code; i++) *outbuf++=*inbuf;
            inbuf++;
        };
    };
}

/*-----------------------------------------------------------------*/
BOOL yw_RCOpenSequence(struct YPASeqData *s)
/*
**  FUNCTION
**      Öffnet IFF-SEQN-File (Filename definiert durch
**      s->fname).
**
**  CHANGED
**      08-Jul-96   floh    created
**      09-Jul-96   floh    Debugging...
**      13-Jul-96   floh    revised & updated
*/
{
    if (s->iff = _AllocIFF()) {
        s->iff->iff_Stream = (ULONG) _FOpen(s->fname,"rb");
        if (s->iff->iff_Stream) {
            _InitIFFasNucleus(s->iff);
            if (_OpenIFF(s->iff,IFFF_READ) == 0) {

                ULONG error;
                struct ContextNode *cn;

                /*** teste, ob SEQN-File ***/
                error = _ParseIFF(s->iff, IFFPARSE_RAWSTEP);
                if (0 == error) {
                    cn = _CurrentChunk(s->iff);
                    if ((ID_FORM==cn->cn_ID)&&(RCIFF_SeqHeader==cn->cn_Type)) {
                        return(TRUE);
                    };
                };
                /*** ab hier Fehler ***/
                _CloseIFF(s->iff);
            };
            _FClose((APTR)s->iff->iff_Stream);
        };
        _FreeIFF(s->iff);
    };
    s->iff = NULL;
    return(FALSE);
}

/*-----------------------------------------------------------------*/
void yw_RCCloseSequence(struct YPASeqData *s)
/*
**  FUNCTION
**      Schließt Sequence-IFF-Stream.
**
**  CHANGED
**      08-Jul-96   floh    created
**      13-Jul-96   floh    revised & updated
*/
{
    if (s->iff) {
        _CloseIFF(s->iff);
        _FClose((APTR)s->iff->iff_Stream);
        _FreeIFF(s->iff);
        s->iff = NULL;
    };
}

/*-----------------------------------------------------------------*/
BOOL yw_RCRewindSequence(struct YPASeqData *s)
/*
**  FUNCTION
**      Schließt und öffnet Sequence-IFF-Stream, macht
**      also effektiv einen Rewind.
**
**  CHANGED
**      08-Jul-96   floh    created
**      13-Jul-96   floh    revised & updated
*/
{
    if (s->iff) yw_RCCloseSequence(s);
    return(yw_RCOpenSequence(s));
}

/*-----------------------------------------------------------------*/
void yw_RCSetPosition(struct ypaworld_data *ywd,
                      struct Bacterium *b,
                      struct flt_triple *p)
/*
**  FUNCTION
**      Ersatz für YBM_SETPOSITION, weil dieses bei
**      den abgeleiteten Klassen ein paar unerwünschte
**      Nebeneffekte hat.
**
**  CHANGED
**      09-Jul-96   floh    created
**      20-Jan-97   floh    behandelt jetzt old_pos korrekt
**                          (notwendig zur Berechnung des DOF
**                          für Audio-Doppler-Effekt).
*/
{
    struct getsectorinfo_msg gsi;

    gsi.abspos_x = p->x;
    gsi.abspos_z = p->z;
    if (_methoda(ywd->world, YWM_GETSECTORINFO, &gsi)) {

        /*** aus alter SektorListe entfernen, in neue einklinken ***/
        if (b->Sector) _Remove((struct Node *) &(b->SectorNode));
        _AddTail((struct List *) &(gsi.sector->BactList),
                 (struct Node *) &(b->SectorNode));

        /*** Positions-Parameter nach Bakterium ***/
        b->Sector  = gsi.sector;
        b->old_pos = b->pos;
        b->pos.x   = p->x;
        b->pos.y   = p->y;
        b->pos.z   = p->z;
        b->SectX   = gsi.sec_x;
        b->SectY   = gsi.sec_y;

    };
}

/*-----------------------------------------------------------------*/
void yw_RCExtractFrame(struct YPASeqData *s)
/*
**  FUNCTION
**      Liest RCIFF_FrameInfo-, RCIFF_ObjectInfo-,
**      RCIFF_AudioInfo- und RCIFF_ModEnergy-Chunks
**      und füllt alle zugehörigen Parameter-Bereiche
**      in <s> mit den gelesenen Daten.
**      IFF-Stream muß gestoppt sein auf RCIFF_FrameHeader
**      FORM des Frames.
**
**  CHANGED
**      09-Jul-96   floh    created
**      13-Jul-96   floh    revised & updated
**      17-Jul-96   floh    guckt jetzt nach, ob ein FLUSH-Chunk
**                          vorhanden ist (generiert von <merge>-Tool
**                          bei Wechsel des Kamera-Standpunktes,
**                          falls vorhanden, wird RCF_Flush gesetzt,
**                          das Flag wird aber in keinem Fall gelöscht!
**                          Damit so ein wichtiges Ereignis nicht verloren
**                          geht, selbst wenn Frame übersprungen werden!
*/
{
    LONG error;

    while ((error = _ParseIFF(s->iff, IFFPARSE_RAWSTEP)) != IFFERR_EOC) {

        struct ContextNode *cn = _CurrentChunk(s->iff);

        /*** RCIFF_Flush ***/
        if (cn->cn_ID == RCIFF_Flush) {
            s->flags |= RCF_Flush;
            _ParseIFF(s->iff, IFFPARSE_RAWSTEP);
            continue;
        };

        /*** RCIFF_FrameInfo ***/
        if (cn->cn_ID == RCIFF_FrameInfo) {
            _ReadChunkBytes(s->iff, &(s->frame), sizeof(struct rc_Frame));
            _ParseIFF(s->iff, IFFPARSE_RAWSTEP);
            continue;
        };

        /*** RCIFF_ObjectInfo ***/
        if (cn->cn_ID == RCIFF_ObjectInfo) {
            _ReadChunkBytes(s->iff, s->buf_objinf, cn->cn_Size);
            s->act_store = cn->cn_Size / sizeof(struct rc_ObjectInfo);
            _ParseIFF(s->iff, IFFPARSE_RAWSTEP);
            continue;
        };

        /*** RCIFF_AudioInfo ***/
        if (cn->cn_ID == RCIFF_AudioInfo) {
            _ReadChunkBytes(s->iff, s->buf_audcod, cn->cn_Size);
            s->act_audcod = cn->cn_Size;
            yw_RCDecodeAudioInfo(s);
            _ParseIFF(s->iff, IFFPARSE_RAWSTEP);
            continue;
        };

        /*** RCIFF_ModEnergy ***/
        if (cn->cn_ID == RCIFF_ModEnergy) {
            _ReadChunkBytes(s->iff, s->buf_modeng, cn->cn_Size);
            s->act_modeng = cn->cn_Size / sizeof(struct rc_ModEnergy);
            _ParseIFF(s->iff, IFFPARSE_RAWSTEP);
            continue;
        };

        /*** unbekannte Chunks ignorieren ***/
        _SkipChunk(s->iff);
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
struct Bacterium *yw_RCNewObject(struct ypaworld_data *ywd, struct rc_ObjectInfo *oi)
/*
**  FUNCTION
**      Erzeugt neues Object aus den in <oi> gegebenen Informationen.
**      Das Objekt wird selbst aus der Totenliste befreit,
**      falls notwendig.
**
**  CHANGED
**      09-Jul-96   floh    created
**      14-Jul-96   floh    setzt jetzt <robo>,<master>,master_bact
**                          in Bacterium-Struktur
**      15-Jul-96   floh    wenn RCTYPE_Vehicle && (oi->proto == 0), wird
**                          handelt es sich um eine virtuelle Kamera, und
**                          kann somit nicht per YWM_CREATEVEHICLE
**                          erzeugt werden, sondern direkt per _new()
**      05-Jun-97   floh    alle Objekte werden jetzt auf BactClassID
**                          BCLID_YPABACT erzeugt
*/
{
    Object *no = NULL;
    struct Bacterium *nb = NULL;

    if (oi->type == RCTYPE_Vehicle) {
        struct createvehicle_msg cvm;
        if (oi->proto) {
            ULONG old_bclid;
            cvm.vp = oi->proto;
            cvm.x  = 0.0;
            cvm.y  = 0.0;
            cvm.z  = 0.0;
            /*** VehicleProto auf BCLID_YPABACT patchen ***/
            old_bclid = ywd->VP_Array[oi->proto].BactClassID;
            ywd->VP_Array[oi->proto].BactClassID = BCLID_YPABACT;
            no = (Object *) _methoda(ywd->world, YWM_CREATEVEHICLE, &cvm);
            ywd->VP_Array[oi->proto].BactClassID = old_bclid;
        } else {
            /*** dat is eine virtuelle Kamera... ***/
            no = _new("ypabact.class", YBA_World, ywd->world, TAG_DONE);
            if (no) {
                _get(no, YBA_Bacterium, &nb);
                _methoda(no, YBM_REINCARNATE, NULL);

                nb->ident = 0;  // !!!
                nb->Owner = 1;

                nb->dir.m11=1.0; nb->dir.m12=0.0; nb->dir.m13=0.0;
                nb->dir.m21=0.0; nb->dir.m22=1.0; nb->dir.m23=0.0;
                nb->dir.m31=0.0; nb->dir.m32=0.0; nb->dir.m33=1.0;
            };
        };
    } else {
        struct createweapon_msg cwm;
        cwm.wp = oi->proto;
        cwm.x  = 0.0;
        cwm.y  = 0.0;
        cwm.z  = 0.0;
        no = (Object *) _methoda(ywd->world, YWM_CREATEWEAPON, &cwm);
    };

    if (no) {
        _get(no, YBA_Bacterium, &nb);
        if (nb->master != 0) _Remove((struct Node *) &(nb->slave_node.nd));
        nb->ident       = oi->id;
        nb->robo        = ywd->UserVehicle;
        nb->master      = ywd->UserVehicle;
        nb->master_bact = ywd->UVBact;
    };

    return(nb);
}

/*-----------------------------------------------------------------*/
void yw_RCUpdateBact(struct ypaworld_data *ywd,
                     struct Bacterium *b,
                     struct rc_ObjectInfo *oi,
                     struct rc_AudioInfo *ai,
                     FLOAT lerp, FLOAT ftime)
/*
**  FUNCTION
**      Überträgt Parameter aus ObjectInfo-Struktur ins
**      gegebene Object, und zwar per Interpolation zwischen
**      momentaner Position und Ausrichtung und der
**      "Zielausrichtung" in der Object-Info-Struktur. Die
**      genaue "Lage" zwischen momentaner und Ziel-Pos
**      definiert <lerp> (0.0 -> momentan, 1.0 -> oi)
**
**          - Position
**          - Rotations-Matrix
**          - gültiger VisProto
**
**  CHANGED
**      09-Jul-96   floh    created
**      13-Jan-97   floh    Rotation wird jetzt als Euler-Angles
**                          gelesen.
**      20-Jan-97   floh    + Audio-Handling
**                          + ftime-Arg (Frametime in Sekunden)
**                            für Berechnung des DOF-Vektors
**      09-Jun-97   floh    + <dof> wurde genau invertiert berechnet,
**                            damit war der Doppler-Effekt genau verkehrt.
*/
{
    Object *visp;
    struct tform *tf;
    struct flt_triple p;
    struct flt_m3x3 oi_m,m;
    FLOAT l;
    EulerAngles ea;
    HMatrix hm;
    ULONG i;

    /*** ermittle neue Position durch Interpolation ***/
    p.x = b->pos.x + ((oi->p.x - b->pos.x) * lerp);
    p.y = b->pos.y + ((oi->p.y - b->pos.y) * lerp);
    p.z = b->pos.z + ((oi->p.z - b->pos.z) * lerp);
    yw_RCSetPosition(ywd,b,&p);

    /*** DOF rückrechnen aus Positions-Änderung über Zeit ***/
    b->dof.x = b->pos.x - b->old_pos.x;
    b->dof.y = b->pos.y - b->old_pos.y;
    b->dof.z = b->pos.z - b->old_pos.z;
    l = nc_sqrt(b->dof.x*b->dof.x + b->dof.y*b->dof.y + b->dof.z*b->dof.z);
    if (l > 0.0) {
        b->dof.x /= l;
        b->dof.y /= l;
        b->dof.z /= l;
        if (ftime > 0.0) b->dof.v = (l / ftime) / METER_SIZE;
        else             b->dof.v = 0;
    } else {
        b->dof.x = 1.0;
        b->dof.y = 0.0;
        b->dof.z = 0.0;
        b->dof.v = 0.0;
    };

    /*** Rotations-Interpolation ***/
    ea.x = (((float)(oi->roll))/127.0)*2*PI;
    ea.y = (((float)(oi->pitch))/127.0)*2*PI;
    ea.z = (((float)(oi->yaw))/127.0)*2*PI;
    ea.w = EulOrdXYZs;
    Eul_ToHMatrix(ea,hm);
    oi_m.m11=hm[0][0]; oi_m.m12=hm[0][1]; oi_m.m13=hm[0][2];
    oi_m.m21=hm[1][0]; oi_m.m22=hm[1][1]; oi_m.m23=hm[1][2];
    oi_m.m31=hm[2][0]; oi_m.m32=hm[2][1]; oi_m.m33=hm[2][2];

    /*** Matrix-Komponenten einzeln interpolieren und renormalisieren ***/
    m.m11 = b->dir.m11 + ((oi_m.m11 - b->dir.m11) * lerp);
    m.m12 = b->dir.m12 + ((oi_m.m12 - b->dir.m12) * lerp);
    m.m13 = b->dir.m13 + ((oi_m.m13 - b->dir.m13) * lerp);
    l = nc_sqrt(m.m11*m.m11 + m.m12*m.m12 + m.m13*m.m13);
    if (l > 0.0) {
        m.m11/=l; m.m12/=l; m.m13/=l;
    } else {
        m.m11=1.0; m.m12=0.0; m.m13=0.0;
    };
    m.m21 = b->dir.m21 + ((oi_m.m21 - b->dir.m21) * lerp);
    m.m22 = b->dir.m22 + ((oi_m.m22 - b->dir.m22) * lerp);
    m.m23 = b->dir.m23 + ((oi_m.m23 - b->dir.m23) * lerp);
    l = nc_sqrt(m.m21*m.m21 + m.m22*m.m22 + m.m23*m.m23);
    if (l > 0.0) {
        m.m21/=l; m.m22/=l; m.m23/=l;
    } else {
        m.m21=0.0; m.m22=1.0; m.m23=0.0;
    };
    m.m31 = b->dir.m31 + ((oi_m.m31 - b->dir.m31) * lerp);
    m.m32 = b->dir.m32 + ((oi_m.m32 - b->dir.m32) * lerp);
    m.m33 = b->dir.m33 + ((oi_m.m33 - b->dir.m33) * lerp);
    l = nc_sqrt(m.m31*m.m31 + m.m32*m.m32 + m.m33*m.m33);
    if (l > 0.0) {
        m.m31/=l; m.m32/=l; m.m33/=l;
    } else {
        m.m31=0.0; m.m32=0.0; m.m33=1.0;
    };
    /*** schlußendlich eintragen ***/
    b->dir = m;

    /*** VisProto ***/
    switch(oi->state) {
        case RCSTATE_Normal:
            visp = b->vis_proto_normal;
            tf   = b->vp_tform_normal;
            break;

        case RCSTATE_Fire:
            visp = b->vis_proto_fire;
            tf   = b->vp_tform_fire;
            break;

        case RCSTATE_Wait:
            visp = b->vis_proto_wait;
            tf   = b->vp_tform_wait;
            break;

        case RCSTATE_Dead:
            visp = b->vis_proto_dead;
            tf   = b->vp_tform_dead;
            break;

        case RCSTATE_MegaDeth:
            visp = b->vis_proto_megadeth;
            tf   = b->vp_tform_megadeth;
            break;

        case RCSTATE_Create:
            visp = b->vis_proto_create;
            tf   = b->vp_tform_create;
            break;

        default:
            visp=NULL; tf=NULL; break;
    };

    if (visp && tf) {
        _method(b->BactObject, OM_SET,
                YBA_VisProto, visp,
                YBA_VPTForm,  tf,
                TAG_DONE);
    };

    /*** Audio-Handling ***/
    b->sc.src[0].pitch = ai->norm_pitch;    // Motoren-Pitch
    for (i=0; i<MAX_SOUNDSOURCES; i++) {

        /*** SoundSource starten? ***/
        if ((ai->active & (1<<i)) && (!(b->sound & (1<<i)))) {
            /*** Aktiv-Merker setzen ***/
            b->sound |= (1<<i);
            _StartSoundSource(&(b->sc),i);
        };

        /*** SoundSource beenden? ***/
        if ((!(ai->active & (1<<i))) && (b->sound & (1<<i))) {
            /*** Aktiv-Merker löschen ***/
            b->sound &= ~(1<<i);
            /*** nur LoopDaLoops müssen manuell beendet werden ***/
            if (b->sc.src[i].flags & AUDIOF_LOOPDALOOP) {
                _EndSoundSource(&(b->sc),i);
            };
        };
    };
}

/*-----------------------------------------------------------------*/
void yw_RCUpdateObjectList(struct ypaworld_data *ywd, 
                           struct YPASeqData *s,
                           FLOAT lerp,
                           ULONG ftime)
/*
**  FUNCTION
**      Vergleicht die Slave-Liste der virtuellen
**      Kamera mit dem <buf_objinf> Puffer. Falls
**      es Unterschiede gibt, werden diese mittels
**      YWM_CREATEVEHICLE und YWM_RELEASEVEHICLE
**      beseitigt.
**
**      Außerdem wird Position, Ausrichtung, Aussehen
**      und Audio(?) bei jedem Objekt auf dem neuesten
**      Stand gehalten.
**
**      Im normalen Player-Betrieb sollte die Routine
**      1x pro Frame (nicht pro Keyframe!) angewendet
**      werden, jeweils mit einem korrektem <lerp>
**      Wert, der definiert, wie nahe der nächste
**      Keyframe (muß bereits in die Puffer geladen worden
**      sein) ist.
**
**      <lerp> Wert:    0.0  -> momentane Position
**                      1.0  -> nächster Keyframe
**
**  CHANGED
**      09-Jul-96   floh    created
**      10-Jul-96   floh    Mit automatischer Interpolation,
**                          behandelt jetzt auch die Objects,
**                          die übereinstimmen.
**      13-jul-96   floh    revised & updated
**      15-Jul-96   floh    böser Bug beim Object-Insert (die
**                          Listenfunktion war komplett fucked up.
**      20-Jan-97   floh    + Übergabe der Frametime, mit der
**                            yw_RCUpdateBact() den DOF korrekt
**                            berechnen kann.
*/
{
    ULONG i;
    struct MinNode *nd;
    FLOAT f_ftime = ((FLOAT)ftime) / 1000.0;

    /*** Pointer auf 1.Bakterie in SlaveList der Kamera ***/
    nd = ywd->UVBact->slave_list.mlh_Head;
    i  = 0;
    while (i < s->act_store) {

        struct Bacterium *b = ((struct OBNode *)nd)->bact;
        struct rc_ObjectInfo *oi = &(s->buf_objinf[i]);
        struct rc_AudioInfo  *ai = &(s->buf_audinf[i]);

        if (nd->mln_Succ) {
            if (oi->id < b->ident) {

                /*** Bacterium erzeugen ***/
                struct Bacterium *nb = yw_RCNewObject(ywd,oi);
                if (nb) {

                    /*** Bakterium positionieren ***/
                    yw_RCUpdateBact(ywd,nb,oi,ai,(FLOAT)1.0,f_ftime);

                    /*** Bacterium von Hand in Geschwader-Liste einklinken ***/
                    nb->slave_node.nd.mln_Pred = nd->mln_Pred;
                    nb->slave_node.nd.mln_Succ = nd;
                    nd->mln_Pred->mln_Succ = &(nb->slave_node.nd);
                    nd->mln_Pred = &(nb->slave_node.nd);

                    /*** <nd> lassen, Index weiterschalten ***/
                    i++;
                };

            } else if (s->buf_objinf[i].id > b->ident) {
                /*** ein (jetzt) überflüssiges Bakterium ***/
                nd = nd->mln_Succ;
                _methoda(ywd->world, YWM_RELEASEVEHICLE, b->BactObject);
                /*** nd ist ok, Index bleibt ***/
            }else{
                /*** Übereinstimmung!!!, <nd> UND Index weiter ***/
                yw_RCUpdateBact(ywd,b,oi,ai,lerp,f_ftime);
                i++;
                nd = nd->mln_Succ;
            };

        } else {

            /*** zuwenig real existierende Slaves, auffüllen ***/
            struct Bacterium *nb = yw_RCNewObject(ywd,oi);
            if (nb) {

                yw_RCUpdateBact(ywd,nb,oi,ai,(FLOAT)1.0,f_ftime);

                /*** an Ende der Geschwader-Liste einklinken! ***/
                _AddTail((struct List *) &(ywd->UVBact->slave_list),
                         (struct Node *) &(nb->slave_node.nd));

                /*** nd auf letztes Element, Index inkrementieren ***/
                nd = nb->slave_node.nd.mln_Succ;
                i++;
            };
        };
    };

    /*** überzählige Slaves noch killen ***/
    while (nd->mln_Succ) {
        struct Bacterium *b = ((struct OBNode *)nd)->bact;
        nd = nd->mln_Succ;
        _methoda(ywd->world, YWM_RELEASEVEHICLE, b->BactObject);
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
BOOL yw_RCSetRandomFrame(struct ypaworld_data *ywd, 
                         struct YPASeqData *s,
                         LONG fnum)
/*
**  FUNCTION
**      Stellt eine definierte Ausgangs-Position her,
**      spult Sequenz-File zurück, geht auf den
**      Frame <fnum>, liest diesen ein und macht
**      ein yw_RCUpdateObjectList() auf den Frame.
**
**      Der Sequenz-IFF-File wird offengelassen und
**      steht auf dem nächsten Frame-Chunk.
**
**      Der 1.Frame ist #0.
**
**  CHANGED
**      09-Jul-96   floh    created
**      10-Jul-96   floh    debugging...
**                          + ywd->TimeStamp wird auf den des
**                            Frames gesetzt.
**      13-Jul-96   floh    revised & updated
*/
{
    ULONG act_frame = 0;

    /*** fnum korrigieren ***/
    if (fnum < 0) fnum=0;
    else if (fnum >= s->num_frames) fnum = s->num_frames-1;

    /*** Sequenz zurückspulen ***/
    if (yw_RCRewindSequence(s)) {

        LONG error;

        /*** suche richtigen Frame-Chunk ***/
        while ((error = _ParseIFF(s->iff,IFFPARSE_RAWSTEP)) != IFFERR_EOC) {

            struct ContextNode *cn = _CurrentChunk(s->iff);

            /*** RCIFF_FrameHeader ***/
            if ((cn->cn_ID == ID_FORM) && (cn->cn_Type == RCIFF_FrameHeader)) {
                /*** richtiger Frame? dann einlesen und zurück ***/
                if (act_frame == fnum) {

                    /*** Frame-Chunks lesen ***/
                    yw_RCExtractFrame(s);

                    /*** TimeStamp rücksetzen ***/
                    ywd->TimeStamp = s->frame.time_stamp;

                    /*** Objectliste resetten ***/
                    yw_RCUpdateObjectList(ywd,s,(FLOAT)1.0,0);

                    /*** und raus ***/
                    return(TRUE);
                };
                /*** diesen Frame skippen ***/
                act_frame++;
                _SkipChunk(s->iff);
                continue;
            };

            _SkipChunk(s->iff);
        };
    };

    /*** Fehler-Ende ***/
    return(FALSE);
}

/*-----------------------------------------------------------------*/
BOOL yw_RCSetNextFrame(struct ypaworld_data *ywd,
                       struct YPASeqData *s)
/*
**  FUNCTION
**      Liest nächsten Frame aus dem offenen IFF-File
**      in die Puffer. Vorher muß yw_RCSetRandomFrame()
**      oder yw_RCSetNextFrame() ausgeführt worden sein
**      (und nix anderes!).
**
**  CHANGED
**      10-Jul-96   floh    created
**      13-Jul-96   floh    revised & updated
*/
{
    LONG error;

    error = _ParseIFF(s->iff,IFFPARSE_RAWSTEP);
    if (error != IFFERR_EOF) {

        struct ContextNode *cn = _CurrentChunk(s->iff);

        /*** RCIFF_FrameHeader ***/
        if ((cn->cn_ID == ID_FORM) && (cn->cn_Type == RCIFF_FrameHeader)) {

            /*** Frame-Chunks lesen ***/
            yw_RCExtractFrame(s);

            /*** ...und raus ***/
            return(TRUE);
        };
    };

    /*** Fehler-Ende ***/
    return(FALSE);
}


/*-----------------------------------------------------------------*/
void yw_RCPlay(struct ypaworld_data *ywd,
               struct YPASeqData *s,
               ULONG ftime)
/*
**  FUNCTION
**      Interpoliert zwischen den Keyframes, lädt bei
**      Bedarf neue Keyframes nach.
**
**  CHANGED
**      10-Jul-96   floh    created
**      13-Jul-96   floh    revised & updated
**      17-Jul-96   floh    behandelt jetzt das Flush-Flag korrekt,
**                          welches von yw_RCSetNextFrame()
**                          (bzw. von yw_RCExtractFrame()) gesetzt wird,
**                          falls ein entsprechender IFF-Chunk durchlaufen
**                          wurde
*/
{
    if (ftime > 0) {

        s->flags &= ~RCF_Flush;

        /*** muß neuer KeyFrame geladen werden? ***/
        while ((s->frame.id != (s->num_frames-1)) &&
               (s->frame.time_stamp < (ywd->TimeStamp+ftime)))
        {
            yw_RCSetNextFrame(ywd,s);
        };
        /*** wenn im Player-Mode und am Ende: an Anfang zurückspulen ***/
        #ifndef YPA_CUTTERMODE
        if (s->frame.id == (s->num_frames-1)) {
            yw_RCSetRandomFrame(ywd,s,0);
            return;
        };
        #endif

        /*** zum nächsten Keyframe "hin-interpolieren" ***/
        if (s->frame.id != (s->num_frames-1)) {

            FLOAT lerp;

            if (s->flags & RCF_Flush) {
                lerp = 1.0;     // keine Interpolation, direkt setzen!
                ywd->TimeStamp = s->frame.time_stamp;
            } else {
                lerp = ((FLOAT)ftime)/((FLOAT)(s->frame.time_stamp - ywd->TimeStamp));
                ywd->TimeStamp += ftime;
            };

            /*** Note: durch ExtractFrame()-Schleife ist garantiert,  ***/
            /*** daß <lerp> zwischen 0.0 und einschließlich 1.0 liegt ***/
            yw_RCUpdateObjectList(ywd,s,lerp,ftime);
        };
    };
}

/*-----------------------------------------------------------------*/
BOOL yw_RCCreateVirtualCamera(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Erzeugt eine virtuelle Kamera ("ypabact.class")
**      und klinkt diese in die CommanderListe des
**      Welt-Objects ein (quasi als Pseudo-Robo).
**      Außerdem werden modifiziert:
**          ywd->UserRobo
**          ywd->UserVehicle
**          ywd->URBact
**          ywd->UVBact
**          ywd->URSlaves
**
**      Der Viewer sitzt IMMER in der Kamera!
**
**  CHANGED
**      09-Jul-96   floh    created
**      15-Jul-96   floh    explizites _SetViewer() auf TForm
**                          der Kamera
*/
{
    Object *co;
    struct Bacterium *cb;
    BOOL retval = FALSE;

    /*** ...siehe yw_deathcache.c/yw_PrivObtainBact() ***/
    co = _new("ypabact.class", YBA_World, ywd->world, TAG_DONE);
    if (co) {

        _get(co, YBA_Bacterium, &cb);
        _methoda(co, YBM_REINCARNATE, NULL);

        cb->ident = 0;  // !!!
        cb->Owner = 1;

        cb->dir.m11=1.0; cb->dir.m12=0.0; cb->dir.m13=0.0;
        cb->dir.m21=0.0; cb->dir.m22=1.0; cb->dir.m23=0.0;
        cb->dir.m31=0.0; cb->dir.m32=0.0; cb->dir.m33=1.0;

        _InitSoundCarrier(&(cb->sc));

        /*** in CommandListe einklinken ***/
        _methoda(ywd->world, YWM_ADDCOMMANDER, co);

        /*** als Viewer schalten ***/
        _set(co, YBA_Viewer, TRUE);
        _set(co, YBA_UserInput, TRUE);
        ywd->UserRobo = co;
        ywd->URBact   = cb;
        ywd->URSlaves = &(cb->slave_list);
        _SetViewer(&(cb->tf));

        /*** success ***/
        retval = TRUE;
    };
    return(retval);
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, yw_YWM_INITPLAYER, struct initplayer_msg *msg)
/*
**  CHANGED
**      08-Jul-96   floh    created
**      13-Jul-96   floh    revised & updated
*/
{
    struct ypaworld_data *ywd = INST_DATA(cl,o);

    if (ywd->in_seq = yw_AllocSequenceData()) {

        struct YPASeqData *s = ywd->in_seq;
        struct createlevel_msg clm;
        struct playercontrol_msg pcm;

        /*** Sequence-Data initialisieren ***/
        strcpy(s->fname,msg->seq_filename);

        /*** TimeStamp zurücksetzen ***/
        ywd->TimeStamp = 0;

        /*** Sequenz-File öffnen ***/
        if (yw_RCOpenSequence(s)) {

            LONG error;

            /*** lese Sequenz-Info und zähle Frames ***/
            while ((error = _ParseIFF(s->iff,IFFPARSE_RAWSTEP)) != IFFERR_EOC) {

                struct ContextNode *cn = _CurrentChunk(s->iff);

                /*** RCIFF_SeqInfo ***/
                if (cn->cn_ID == RCIFF_SeqInfo) {
                    _ReadChunkBytes(s->iff, &(s->seq), sizeof(struct rc_Sequence));
                    _ParseIFF(s->iff,IFFPARSE_RAWSTEP); // EOC
                    continue;
                };

                /*** RCIFF_FrameHeader ***/
                if ((cn->cn_ID == ID_FORM) && (cn->cn_Type == RCIFF_FrameHeader)) {
                    s->num_frames++;
                    /*** restliche FORM mit Subchunks überspringen ***/
                    _SkipChunk(s->iff);
                    continue;
                };

                _SkipChunk(s->iff);
            };
        } else return(FALSE);   // FEHLER

        /*** Sequenz-File erstmal wieder schließen ***/
        yw_RCCloseSequence(s);

        /*** Level erzeugen ***/
        clm.level_num  = s->seq.level;
        clm.level_mode = LEVELMODE_SEQPLAYER;
        if (_methoda(o, YWM_CREATELEVEL, &clm)) {

            /*** virtuelle Kamera erzeugen ***/
            if (!yw_RCCreateVirtualCamera(ywd)) {
                _LogMsg("PLAYER ERROR: could not create virtual camera!\n");
                _methoda(o, YWM_KILLPLAYER, NULL);
                return(FALSE);
            };

            /*** Kamera ausrichten ***/
            s->cam_pos.x=0.0; s->cam_pos.y=0.0; s->cam_pos.z=0.0;
            s->cam_mx.m11=1.0; s->cam_mx.m12=0.0; s->cam_mx.m13=0.0;
            s->cam_mx.m21=0.0; s->cam_mx.m22=1.0; s->cam_mx.m23=0.0;
            s->cam_mx.m31=0.0; s->cam_mx.m32=0.0; s->cam_mx.m33=1.0;

            /*** definierte Anfangssituation auf 1.Frame ***/
            if (!yw_RCSetRandomFrame(ywd,s,0)) {
                _LogMsg("PLAYER ERROR: could not position on 1st frame!\n");
                _methoda(o, YWM_KILLPLAYER, NULL);
                return(FALSE);
            };

        } else return(FALSE);

        /*** und gleich anfangen zu spielen... ***/
        pcm.mode = PLAYER_PLAY;
        pcm.arg0 = 0;
        _methoda(o, YWM_PLAYERCONTROL, &pcm);

        pcm.mode = CAMERA_PLAYER;
        pcm.arg0 = 0;
        _methoda(o, YWM_PLAYERCONTROL, &pcm);

    } else return(FALSE);

    /*** success ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, yw_YWM_KILLPLAYER, void *nil)
/*
**  CHANGED
**      08-Jul-96   floh    created
**      13-Jul-96   floh    revised & updated
*/
{
    struct ypaworld_data *ywd = INST_DATA(cl,o);

    if (ywd->in_seq) {

        struct YPASeqData *s = ywd->in_seq;

        /*** schließe IFF-Stream ***/
        yw_RCCloseSequence(s);

        /*** Level killen ***/
        _methoda(o, YWM_KILLLEVEL, NULL);

        /*** SeqData-Struktur freigeben ***/
        yw_FreeSequenceData(s);
        ywd->in_seq = NULL;
    };
}

/*-----------------------------------------------------------------*/
_dispatcher(void, yw_YWM_PLAYERCONTROL, struct playercontrol_msg *msg)
/*
**  CHANGED
**      10-Jul-96   floh    created
**      13-Jul-96   floh    revised & updated
**      17-Jul-96   floh    ändert jetzt auch SR.ActiveMode
**      18-Jul-96   floh    Bugfix: wenn in Player-Modus geschaltet
**                          wird Pos und Matrix normalisiert
**      14-Jan-97   floh    + PLAYER_RELPOS
*/
{
    struct ypaworld_data *ywd = INST_DATA(cl,o);
    struct YPASeqData *s = ywd->in_seq;

    /*** wird aus OnBoard-Modus in freien Modus zurückgeschaltet? ***/
    if (((s->camera_mode==CAMERA_ONBOARD1)||
         (s->camera_mode==CAMERA_ONBOARD2)||
         (s->camera_mode==CAMERA_PLAYER)) &&
        ((msg->mode == CAMERA_FREESTATIV) ||(msg->mode == CAMERA_LINKSTATIV)))
    {
        /*** dann Kamera-Position "rück-globalisieren" ***/
        s->cam_pos = ywd->UVBact->pos;
        s->cam_mx  = ywd->UVBact->dir;
    };

    switch(msg->mode) {
        case PLAYER_STOP:
        case PLAYER_PLAY:
            s->player_mode = msg->mode;
            break;

        case PLAYER_GOTO:
            yw_RCSetRandomFrame(ywd,s,msg->arg0);
            break;

        case PLAYER_NEXT:
            yw_RCSetRandomFrame(ywd,s,s->frame.id+1);
            break;

        case PLAYER_PREV:
            yw_RCSetRandomFrame(ywd,s,s->frame.id-1);
            break;

        case PLAYER_RELPOS:
            yw_RCSetRandomFrame(ywd,s,s->frame.id+msg->arg0);
            break;

        case CAMERA_FREESTATIV:
        case CAMERA_LINKSTATIV:
            s->camera_mode = msg->mode;
            s->camera_link = 0;
            break;

        case CAMERA_ONBOARD1:
        case CAMERA_ONBOARD2:
        case CAMERA_PLAYER:
            s->camera_mode = msg->mode;
            s->camera_link = msg->arg0;
            s->cam_pos.x = 0.0; s->cam_pos.y = 0.0; s->cam_pos.z = 0.0;
            s->cam_mx.m11 = 1.0; s->cam_mx.m12 = 0.0; s->cam_mx.m13 = 0.0;
            s->cam_mx.m21 = 0.0; s->cam_mx.m22 = 1.0; s->cam_mx.m23 = 0.0;
            s->cam_mx.m31 = 0.0; s->cam_mx.m32 = 0.0; s->cam_mx.m33 = 1.0;
            break;

        default:
            s->player_mode = PLAYER_STOP;
            break;
    };

    /*** das war's bereits! ***/
}

/*-----------------------------------------------------------------*/
void yw_RCRotateCamera(struct ypaworld_data *ywd,
                       FLOAT ftime,
                       struct VFMInput *ip)
/*
**  FUNCTION
**      Rotiert Kamera per UserInput (Slider 10 & 11).
**      Das ganze frei nach AF's HandleInput-Code der
**      ypagun.class.
**
**  CHANGED
**      13-Jul-96   floh    created
**      15-Jul-96   floh    manipuliert nicht mehr Kamera-Bakterium
**                          direkt, sondern die extra Kamera-Matrix
*/
{
    FLOAT gy_rot,lx_rot;

    /*** Drehung um globales Y ***/
    gy_rot = ip->Slider[10] * 2.5 * ftime;
    if (fabs(gy_rot) > 0.001) yw_RCRotGlobalY(&(ywd->in_seq->cam_mx),gy_rot);

    /*** Drehung um lokales X ***/
    lx_rot = ip->Slider[11] * 2.5 * ftime;
    if (fabs(lx_rot) > 0.001) yw_RCRotLocalX(&(ywd->in_seq->cam_mx),lx_rot);

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yw_RCTranslateCamera(struct ypaworld_data *ywd,
                          FLOAT ftime,
                          struct VFMInput *ip)
/*
**  FUNCTION
**      Verschiebt Kamera per UserInput.
**
**  CHANGED
**      13-Jul-96   floh    created
**      15-Jul-96   floh    manipuliert nicht mehr Kamera-Bakterium
**                          direkt, sondern die extra Kamera-Pos
*/
{
    struct flt_triple l;
    struct flt_triple *p;
    struct flt_m3x3 *m;
    FLOAT dx,dz;
    FLOAT len;

    p = &(ywd->in_seq->cam_pos);
    m = &(ywd->in_seq->cam_mx);

    l.x = ip->Slider[SL_FLY_DIR]     * 250.0 * ftime;  // links/rechts
    l.y = -ip->Slider[SL_FLY_SPEED]  * 250.0 * ftime;  // hoch/runter
    l.z = -ip->Slider[SL_FLY_HEIGHT] * 150.0 * ftime;  // vorwärts/rückwärts

    /*** vorwärts/rückwärts ***/
    dx = m->m31;
    dz = m->m33;
    len = nc_sqrt(dx*dx + dz*dz);
    if (len > 0.0) { dx/=len; dz/=len; };

    p->x += dx*l.z;
    p->z += dz*l.z;

    /*** links/rechts ***/
    dx = m->m11;
    dz = m->m13;
    len = nc_sqrt(dx*dx + dz*dz);
    if (len > 0.0) { dx/=len; dz/=len; };
    p->x += dx*l.x;
    p->z += dz*l.x;

    /*** hoch/runter ***/
    p->y += l.y;
}

/*-----------------------------------------------------------------*/
struct Bacterium *yw_RCFindByID(struct ypaworld_data *ywd, LONG id)
/*
**  FUNCTION
**      Durchsucht Slave-List der Kamera nach Bacterium
**      mit gegebener ID.
**
**  CHANGED
**      15-Jul-96   floh    created
*/
{
    struct MinList *ls = ywd->URSlaves;
    struct MinNode *nd;
    for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {
        struct Bacterium *b = ((struct OBNode *)nd)->bact;
        if (b->ident == id) return(b);
    };
    return(NULL);
}

/*-----------------------------------------------------------------*/
void yw_RCCameraPrepPublish(struct ypaworld_data *ywd,
                            struct YPASeqData *s,
                            struct Bacterium *b,
                            struct VFMInput *ip)
/*
**  FUNCTION
**      Spezieller Ersatz für YBM_TR_LOGIC auf der virtuellen
**      Kamera, übernimmt u.a. Input-Handling für die Kamera
**      und die Realisierung der diversen Kamera-Modi.
**
**  CHANGED
**      13-Jul-96   floh    created
**      14-Jul-96   floh    + ControlLock-Zeugs
**      15-Jul-96   floh    + Modifikationen
**                          + CAMERA_ONBOARD1-Modus
**                          + Snap-Back auf button[0] (Bremse)
**                          + CAMERA_PLAYER-Modus
*/
{
    FLOAT ftime = (((FLOAT)ip->FrameTime) / 1000.0);
    struct ClickInfo *ci = &(ip->ClickInfo);
    FLOAT l;

    /*** ControlLock ein, wenn RMB down über "ungefährlichem" Gebiet ***/
    if ((ywd->ControlLock == FALSE) && (ci->flags & CIF_RMOUSEDOWN)) {
        if ((ci->box!=&(MR.req.req_cbox))&&(ci->box!=&(FR.l.Req.req_cbox))) {
            ywd->ControlLock = TRUE;
        };
    } else if ((ywd->ControlLock) && (ci->flags & CIF_RMOUSEDOWN)) {
        /*** ControlLock aus... ***/
        ywd->ControlLock = FALSE;
    };

    /*** Snap back ***/
    if (ip->Buttons & (1<<0)) {
        s->cam_mx.m11=1.0; s->cam_mx.m12=0.0; s->cam_mx.m13=0.0;
        s->cam_mx.m21=0.0; s->cam_mx.m22=1.0; s->cam_mx.m23=0.0;
        s->cam_mx.m31=0.0; s->cam_mx.m32=0.0; s->cam_mx.m33=1.0;
    };

    /*** Kamera "lokal" per UserInput bewegen ***/
    yw_RCTranslateCamera(ywd,ftime,ip);
    if (ywd->ControlLock) yw_RCRotateCamera(ywd,ftime,ip);

    /*** Kamera-Verhalten je nach Modus ***/
    switch(s->camera_mode) {

        case CAMERA_FREESTATIV:
            /*** Kamera-Ausrichtung und -Position direkt übernehmen ***/
            yw_RCSetPosition(ywd,b,&(s->cam_pos));
            b->dir = s->cam_mx;
            break;

        case CAMERA_ONBOARD1:
            {
                /*** Kamera-Ausrichtung und -Position lokal zu Linked Object ***/
                struct Bacterium *link = yw_RCFindByID(ywd,s->camera_link);
                if (link) {
                    /*** globale Position errechnen ***/
                    struct flt_triple *p = &(s->cam_pos);
                    struct flt_triple np;
                    struct flt_m3x3 *m = &(link->dir);
                    np.x = m->m11*p->x + m->m21*p->y + m->m31*p->z + link->pos.x;
                    np.y = m->m12*p->x + m->m22*p->y + m->m32*p->z + link->pos.y;
                    np.z = m->m13*p->x + m->m23*p->y + m->m33*p->z + link->pos.z;
                    yw_RCSetPosition(ywd,b,&np);

                    /*** globale Matrix errechnen ***/
                    nc_m_mul_m(&(s->cam_mx),&(link->dir),&(b->dir));
                };
            };
            break;

        case CAMERA_PLAYER:
            {
                /*** Kamera-Ausrichtung und -Position lokal zu Viewer Object ***/
                struct Bacterium *vwr = yw_RCFindByID(ywd,s->frame.viewer);
                if (vwr) {
                    /*** globale Position errechnen ***/
                    struct flt_triple *p = &(s->cam_pos);
                    struct flt_triple np;
                    struct flt_m3x3 *m = &(vwr->dir);
                    np.x = m->m11*p->x + m->m21*p->y + m->m31*p->z + vwr->pos.x;
                    np.y = m->m12*p->x + m->m22*p->y + m->m32*p->z + vwr->pos.y;
                    np.z = m->m13*p->x + m->m23*p->y + m->m33*p->z + vwr->pos.z;
                    yw_RCSetPosition(ywd,b,&np);

                    /*** globale Matrix errechnen ***/
                    nc_m_mul_m(&(s->cam_mx),&(vwr->dir),&(b->dir));
                };
            };
            break;
    };

    /*** DOF rückrechnen aus Positions-Änderung über Zeit ***/
    b->dof.x = b->old_pos.x - b->pos.x;
    b->dof.y = b->old_pos.y - b->pos.y;
    b->dof.z = b->old_pos.z - b->pos.z;
    l = nc_sqrt(b->dof.x*b->dof.x + b->dof.y*b->dof.y + b->dof.z*b->dof.z);
    if (l > 0.0) {
        b->dof.x /= l;
        b->dof.y /= l;
        b->dof.z /= l;
        if (ftime > 0.0) b->dof.v = (l / ftime) / METER_SIZE;
        else             b->dof.v = 0;
    } else {
        b->dof.x = 1.0;
        b->dof.y = 0.0;
        b->dof.z = 0.0;
        b->dof.v = 0.0;
    };

    /*** resultierende Position und Matrix nach TForm ***/
    b->tf.loc   = b->pos;
    b->tf.loc_m = b->dir;
}

/*-----------------------------------------------------------------*/
void yw_RCPreparePublish(struct Bacterium *b)
/*
**  FUNCTION
**      Ein Ersatz für YBM_TR_LOGIC. Mal sehen, was alles
**      so anfällt...
**      Muß auf jede Bakterie einzeln angewendet werden,
**      und NICHT auf virtuelle Kamera (dafür gibts eine
**      extra Routine).
**
**  CHANGED
**      10-Jul-96   floh    created
*/
{
    /*** Position und transponierte Matrix nach TForm ***/
    b->tf.loc = b->pos;

    b->tf.loc_m.m11 = b->dir.m11;
    b->tf.loc_m.m12 = b->dir.m21;
    b->tf.loc_m.m13 = b->dir.m31;

    b->tf.loc_m.m21 = b->dir.m12;
    b->tf.loc_m.m22 = b->dir.m22;
    b->tf.loc_m.m23 = b->dir.m32;

    b->tf.loc_m.m31 = b->dir.m13;
    b->tf.loc_m.m32 = b->dir.m23;
    b->tf.loc_m.m33 = b->dir.m33;

    /*** SoundCarrier updaten (FIXME: _RefreshSoundCarrier + Vwr-Versatz) ***/
    b->sc.pos = b->pos;
    b->sc.vec.x = b->dof.x * b->dof.v;
    b->sc.vec.y = b->dof.y * b->dof.v;
    b->sc.vec.z = b->dof.z * b->dof.v;
    _RefreshSoundCarrier(&(b->sc));
}

/*-----------------------------------------------------------------*/
_dispatcher(void, yw_YWM_TRIGGERPLAYER, struct trigger_msg *msg)
/*
**  FUNCTION
**      Player-Version von yw_BSM_TRIGGER.
**
**  CHANGED
**      10-Jul-96   floh    created
**      15-Jul-96   floh    + Recorder-Zeugs
**                          + alle "Digi-Features" stehen auch
**                            hier zur Verfügung (nur im CutterMode)
**      19-Aug-96   floh    + neues Render-Handling
**      25-Nov-96   floh    + _BeginRefresh()/_EndRefresh() gekillt
**
**  NOTE
**      siehe auch yw_sim.c/yw_BSM_TRIGGER()!
*/
{
    struct ypaworld_data *ywd = INST_DATA(cl,o);
    struct YPASeqData *s = ywd->in_seq;
    struct flt_triple vwr_vec;
    struct MinList *ls;
    struct MinNode *nd;
    struct flt_m3x3 *rm;
    struct flt_m3x3 nm;
    struct tform *vwr;

    /*** Profile Data ***/
    ULONG prof_frametime;
    ULONG prof_rendering;

    /*** PROFILE ***/
    prof_frametime = yw_StartProfile();

    /*** GfxObject Pointer besorgen ***/
    _OVE_GetAttrs(OVET_Object, &(ywd->GfxObject), TAG_DONE);

    /*** Frame-Initialisierungs-Sachen ***/
    ywd->TriggerMsg  = msg;
    ywd->FrameCount++;
    ywd->TLMsg.user_action = YW_ACTION_NONE;
    ywd->TLMsg.global_time = msg->global_time;
    ywd->TLMsg.frame_time  = msg->frame_time;
    ywd->TLMsg.input       = msg->input;
    ywd->TLMsg.slave_count = 0;
    ywd->FramesProSec = 1024 / msg->frame_time;
    ywd->Profile[PROF_FRAME] = ywd->FramesProSec;

    /*** schon mal anfangen, den Schirm zu löschen ***/
    _methoda(ywd->GfxObject,DISPM_Begin,0);
    _set(ywd->GfxObject, RASTA_BGPen, 0);
    _methoda(ywd->GfxObject, RASTM_Clear, NULL);

    /*** wer weiß, wozu wir's mal noch brauchen... ***/
    yw_RemapCommanders(ywd);

    /*** Cutter-Only-Stuff ***/
    #ifdef YPA_CUTTERMODE
    yw_ClearFootPrints(ywd);
    yw_LayoutGUI(o,ywd);
    yw_HandleGUIInput(ywd,msg->input);
    yw_GetRealViewerPos(ywd);
    yw_RenderMap(ywd);
    yw_BuildTrLogicMsg(o,ywd,msg->input);
    /*** falls Recording, neuen Frame starten ***/
    if (ywd->out_seq->active) yw_RCNewFrame(ywd,msg->frame_time);
    #endif

    /*** Visier ausschalten (kann per YWM_VIZOR eingeschaltet werden) ***/
    ywd->visor.mg_type  = VISORTYPE_NONE;
    ywd->visor.gun_type = VISORTYPE_NONE;

    /*** Keyframes interpolieren und nachladen ***/
    if (s->player_mode != PLAYER_STOP) yw_RCPlay(ywd,s,msg->frame_time);

    /*** jede Bakterie auf Publishing vorbereiten ***/
    /*** (Ersatz für YBM_TR_LOGIC)                ***/
    yw_RCCameraPrepPublish(ywd,s,ywd->UVBact,msg->input);

    /*** Audio-Frame-Initialisierung ***/
    vwr_vec.x = ywd->UVBact->dof.x * ywd->UVBact->dof.v;
    vwr_vec.y = ywd->UVBact->dof.y * ywd->UVBact->dof.v;
    vwr_vec.z = ywd->UVBact->dof.z * ywd->UVBact->dof.v;
    _StartAudioFrame(msg->frame_time,
                     &(ywd->UVBact->pos),
                     &(vwr_vec),
                     &(ywd->UVBact->dir));

    ls = &(ywd->UVBact->slave_list);
    for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {
        struct Bacterium *b = ((struct OBNode *)nd)->bact;
        yw_RCPreparePublish(b);
    };

    /*** Audio-Frame beenden, Shake-FX beachten ***/
    rm  = _EndAudioFrame();
    vwr = _GetViewer();
    nc_m_mul_m(rm,&(vwr->loc_m),&nm);
    vwr->loc_m = nm;

    #ifdef YPA_CUTTERMODE
    /*** falls Recording, Frame beenden und nach File flushen ***/
    if (ywd->out_seq->active) yw_RCEndFrame(ywd);
    #endif

    /*** Frame rendern ***/
    prof_rendering = yw_StartProfile();
    #ifdef YPA_CUTTERMODE
    yw_RenderFrame(o,ywd,msg,TRUE);
    #else
    yw_RenderFrame(o,ywd,msg,FALSE);
    #endif
    yw_PutDebugInfo(ywd,msg->input);
    _methoda(ywd->GfxObject, DISPM_End, NULL);
    ywd->Profile[PROF_RENDERING] = yw_EndProfile(prof_rendering);

    /*** Snapshot, Recording an/aus ***/
    yw_DoDigiStuff(ywd, msg->input);

    /*** Ende ***/
    ywd->Profile[PROF_FRAMETIME] = yw_EndProfile(prof_frametime);
    ywd->Profile[PROF_POLYGONS]  = ywd->PolysDrawn;
    yw_TriggerProfiler(ywd);
}

