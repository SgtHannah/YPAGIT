/*
**  $Source: PRG:VFM/Classes/_AreaClass/ac_publex.c,v $
**  $Revision: 38.19 $
**  $Date: 1998/01/06 14:38:38 $
**  $Locker:  $
**  $Author: floh $
**
**  ADEM_EXPRESS und ADEM_PUBLISH Dispatcher der
**  area.class, jetzt in C.
**
**  (C) Copyright 1994 by A.Weissflog
*/
#define NOMATH_FUNCTIONS 1

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "engine/engine.h"
#include "polygon/polygonflags.h"
#include "polygon/polygon.h"

#include "bitmap/rasterclass.h"
#include "skeleton/skltclass.h"
#include "ade/areaclass.h"

/*** externe Symbole ***/
_extern_use_nucleus
_extern_use_ov_engine

#define LNN_CODE    (PLGF_LINEARMAPPED|PLGF_NOSHADE|PLGF_NOTRACY)
#define LNC_CODE    (PLGF_LINEARMAPPED|PLGF_NOSHADE|PLGF_CLEARTRACY)
#define LGN_CODE    (PLGF_LINEARMAPPED|PLGF_GRADSHADE|PLGF_NOTRACY)
#define LGC_CODE    (PLGF_LINEARMAPPED|PLGF_GRADSHADE|PLGF_CLEARTRACY)

#define ZNN_CODE    (PLGF_DEPTHMAPPED|PLGF_NOSHADE|PLGF_NOTRACY)
#define ZNC_CODE    (PLGF_DEPTHMAPPED|PLGF_NOSHADE|PLGF_CLEARTRACY)
#define ZGN_CODE    (PLGF_DEPTHMAPPED|PLGF_GRADSHADE|PLGF_NOTRACY)
#define ZGC_CODE    (PLGF_DEPTHMAPPED|PLGF_GRADSHADE|PLGF_CLEARTRACY)

#define LNF_CODE    (PLGF_LINEARMAPPED|PLGF_FLATTRACY)
#define NNN_CODE    (0)

/*-----------------------------------------------------------------*/
_dispatcher(void, area_ADEM_PUBLISH, struct publish_msg *msg)
/*
**  CHANGED
**      21-Oct-94   floh    created + debugged.
**      22-Oct-94   floh    Ooops, da war doch noch so ein kleiner
**                          aber fieser Bug in der MaxZ-Ermittlung...
**      29-Oct-94   floh    akzeptiert jetzt (vorübergehend) eine
**                          <struct transform_msg> mit
**                          Frame-Time-Information und benutzt
**                          diese Frametime fürs BMM_GETBITMAP.
**      21-Nov-94   floh    (1) jetzt mit <publish_msg>
**                          (2) KEIN serial debug code
**                          (3) __geta4
**      18-Jan-95   floh    Kompletter Re-Write wegen massiven Änderungen
**                          in ADEM_PUBLISH-Handling (Publish- und
**                          Args-Stack).
**                          Außerdem [bei der Gelegenheit] Nucleus2-Revision.
**      20-Jan-95   floh    Gibt jetzt keinen Pointer auf den ArgStack
**                          zurück, sondern schreibt die beiden Stack-Ptr
**                          zurück in die <msg>, so sie modifiziert wurden.
**                          Aus diesem Grund ist es relativ sinnlos,
**                          ADEM_PUBLISH mit _method() zu verwenden, weil
**                          die <msg> in diesem Fall verlorengeht (weil sie
**                          ja auf dem Stack aufgebaut wurde). Stattdessen
**                          mit _methoda() und einer statischen Msg arbeiten!
**      16-Mar-95   floh    (1) Die <publish_msg> hat jetzt ein neues Feld
**                          namens <ade_count>, jedes ADE incrementiert
**                          diesen Wert für die Anzahl der "Einzel-Elemente",
**                          die bearbeitet wurden. Als Benchmark-Feedback.
**                          (2) Kleinere Peephole-Optimierungen.
**      30-Jun-95   floh    verändertes Depth-Fading-Verhalten
**                          (dfade_start, dfade_length).
**      02-Jul-95   floh    Depthfading jetzt "rund"
**      07-Jul-95   floh    (1) Map-Koordinaten-Definition jetzt per
**                              VFMOutline, Polygon-Definition jetzt
**                              mit Point2D's
**                          (2) publish_msg jetzt mit <time_stamp>
**      02-Oct-95   floh    Peephole-Optimierungen, vor allem
**                          nicht mehr genutzte Parameter raus.
**      09-Nov-95   floh    Es gibt jetzt 2 Pubstacks (siehe base.class).
**                          Durchsichtige Polygone werden auf den Back-To-
**                          Front-Stack gepusht, alles was solid ist,
**                          kommt in den Front-To-Back-Stack!
**      12-Nov-95   floh    One PubStack!!!
**      17-Jan-96   floh    revised & updated (alles, was mit
**                          PolygonInfo und PolyPipe() zu tun hat, wird
**                          wahrscheinlich demnächst in die skeleton.class
**                          verlagert werden!
**      01-Apr-96   floh    Flächen, die durchs Depthfading schwarz sind,
**                          werden nicht mehr weggeschaltet, statt dessen
**                          werden sie als "Flat-Zero-Poly" gezeichnet.
**      11-Jun-96   floh    mehr oder weniger komplett umgeschrieben,
**                          weil jetzt SKLM_EXTRACTPOLY der skeleton.class
**                          benutzt wird.
**      15-Apr-97   floh    + ade_count wird nur für sichtbare Polys
**                            hochgezählt
*/
{
    struct sklt_extract_msg sex;
    struct rast_poly *rp = (struct rast_poly *) (msg->argstack+1);
    struct area_data *ad = INST_DATA(cl,o);
    void *next;
    ULONG pflags;

    /*** rast_poly-Struktur weitgehend initialisieren ***/
    pflags = ad->PolyInfo.Flags;
    pflags &= ~(PLGF_SCANLINE|PLGF_TXTBIT1|PLGF_TRACYBIT3);
    switch (pflags) {
        case LNN_CODE: pflags=RPF_LinMap; break;
        case LNC_CODE: pflags=RPF_LinMap|RPF_ZeroTracy; break;
        case LGN_CODE: pflags=RPF_LinMap|RPF_GradShade; break;
        case LGC_CODE: pflags=RPF_LinMap|RPF_GradShade|RPF_ZeroTracy; break;
        case ZNN_CODE: pflags=RPF_PerspMap; break;
        case ZNC_CODE: pflags=RPF_PerspMap|RPF_ZeroTracy; break;
        case ZGN_CODE: pflags=RPF_PerspMap|RPF_GradShade; break;
        case ZGC_CODE: pflags=RPF_PerspMap|RPF_GradShade|RPF_ZeroTracy; break;
        case LNF_CODE: pflags=RPF_LinMap|RPF_LUMTracy; break;
        case NNN_CODE: pflags=0; break;
        default: return;
    };
    rp->flags = pflags;
    rp->xyz   = (struct rast_xyz *) (rp+1); // unmittelbar nach rast_poly!

    /*** sklt_extract_msg ausfüllen ***/
    sex.pnum  = ad->PolyInfo.PolyNum;
    sex.flags = 0;                            
    if (rp->flags & (RPF_LinMap|RPF_PerspMap))     sex.flags |= SKLF_UV;
    if (rp->flags & (RPF_FlatShade|RPF_GradShade)) sex.flags |= SKLF_BRIGHT;
    if (ad->Flags & (AREAF_DepthFade))             sex.flags |= SKLF_DFADE;
    sex.poly  = rp;
    sex.min_z = msg->min_z;
    sex.max_z = msg->max_z;
    sex.shade = ((FLOAT)ad->ShadeValue)/256.0;
    sex.dfade_start = msg->dfade_start;
    sex.dfade_len   = msg->dfade_length;
    if (ad->TxtBitmap) {
        struct bmpobtain_msg bom;
        bom.time_stamp = msg->time_stamp;
        bom.frame_time = msg->frame_time;
        _methoda(ad->TxtBitmap, BMM_BMPOBTAIN, &bom);
        rp->map[0] = bom.bitmap;
        sex.uv     = bom.outline;
    } else {
        rp->map[0] = NULL;
        sex.uv     = NULL;
    };
    if (next = (void *) _methoda(msg->sklto, SKLM_EXTRACTPOLY, &sex)) {

        /*** Polygon ist sichtbar! ***/
        FLOAT max_z;
        ULONG i;

        /*** nur sichtbare Polygone hochzählen ***/
        msg->ade_count++;

        /*** NoShade-Optimierung ***/
        if (rp->flags & (RPF_GradShade|RPF_FlatShade)) {
            ULONG full_black = 0;
            ULONG full_white = 0;
            for (i=0; i<rp->pnum; i++) {
                if (rp->b[i] < 0.01)      full_white++;
                else if (rp->b[i] > 0.99) full_black++;
            };
            if (full_white == i) rp->flags &= ~(RPF_GradShade|RPF_FlatShade);
            else if (full_black == i) rp->flags = 0;
        };

        /** größtes Z finden ***/
        max_z = 0.0;
        for (i=0; i<rp->pnum; i++) {
            if (rp->xyz[i].z > max_z) max_z = rp->xyz[i].z;
        };

        /*** Publish-Stack-Element füllen ***/
        msg->pubstack->depth = FLOAT_TO_INT(max_z);
        msg->pubstack->args  = msg->argstack;
        msg->pubstack++;

        /*** Arg-Stack-Element auffüllen und weiterschalten ***/
        msg->argstack->draw_func = (void(*)(void *)) _DrawPolygon;
        msg->argstack = next;
    };
}

