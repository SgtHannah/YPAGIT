/*
**  $Source: PRG:VFM/Classes/_AMeshClass/amesh_publish.c,v $
**  $Revision: 38.13 $
**  $Date: 1998/01/06 14:37:14 $
**  $Locker:  $
**  $Author: floh $
**
**  ADEM_PUBLISH für amesh.class
**
**  (C) Copyright 1995 by A.Weissflog
*/
#define NOMATH_FUNCTIONS 1

#include <exec/types.h>

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "engine/engine.h"
#include "polygon/polygonflags.h"
#include "polygon/polygon.h"

#include "bitmap/rasterclass.h"
#include "skeleton/skltclass.h"
#include "ade/ameshclass.h"

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
_dispatcher(void, amesh_ADEM_PUBLISH, struct publish_msg *msg)
/*
**  FUNCTION
**      Modifizierter ADEM_PUBLISH Dispatcher der amesh.class.
**      Die Bitmap-Information wird (falls überhaupt notwendig)
**      nur einmal am Anfang geholt, danach werden alle
**      Einzel-Polygone durchgerasselt. Dabei wird direkt die
**      Polygon-Info-Struktur in der LID der area.class gepatcht und
**      verwendet.
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      15-Mar-95   floh    created
**      30-Jun-95   floh    Depthfading jetzt neu
**      02-Jul-95   floh    Depthfading jetzt "rund"
**      08-Jul-95   floh    (1) Map-Koordinaten-Definition jetzt per
**                              <struct VFMOutline>
**                          (2) publish_msg jetzt mit <time_stamp>
**      02-Oct-95   floh    diverse Peephole-Optimierungen
**      09-Nov-95   floh    Es gibt jetzt 2 Pubstacks (siehe base.class).
**                          Durchsichtige Polygone werden auf den Back-To-
**                          Front-Stack gepusht, alles was solid ist,
**                          kommt in den Front-To-Back-Stack!
**      12-Nov-95   floh    One PubStack!
**      17-Jan-96   floh    revised & updated
**      01-Apr-96   floh    Flächen, die durchs Depthfading schwarz sind,
**                          werden nicht mehr weggeschaltet, statt dessen
**                          werden sie als "Flat-Zero-Poly" gezeichnet.
**      11-Jun-96   floh    umgeschrieben, arbeitet jetzt mit
**                          SKLM_EXTRACTPOLY der skeleton.class
**      15-Apr-97   floh    Bugfix: msg->ade_count wurde für alle
**                          ADEs, nicht nur die sichtbaren hochgezählt...
**
**  SEE ALSO
**      _AreaClass/ac_publex.c/area_ADEM_PUBLISH()
*/
{
    ULONG i,pflags;
    struct VFMBitmap *txtbmp;
    struct amesh_data *amd   = INST_DATA(cl,o);
    struct sklt_extract_msg sex;

    /*** Polygon-Flags konvertieren ***/
    pflags = amd->PolyInfo->Flags;
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

    /*** sklt_extract_msg vorinitialisieren ***/
    sex.flags = 0;
    if (pflags & (RPF_LinMap|RPF_PerspMap))     sex.flags |= SKLF_UV;
    if (pflags & (RPF_FlatShade|RPF_GradShade)) sex.flags |= SKLF_BRIGHT;
    if (amd->Flags & AMESHF_DepthFade)          sex.flags |= SKLF_DFADE;
    sex.min_z = msg->min_z;
    sex.max_z = msg->max_z;
    sex.dfade_start = msg->dfade_start;
    sex.dfade_len   = msg->dfade_length;
    if (amd->TxtBitmap) {
        struct bmpobtain_msg bom;
        bom.time_stamp = msg->time_stamp;
        bom.frame_time = msg->frame_time;
        _methoda(amd->TxtBitmap, BMM_BMPOBTAIN, &bom);
        txtbmp = bom.bitmap;
    } else {
        txtbmp = NULL;
    };

    /*** einen Polygon nach dem anderen abhandeln... */
    for (i=0; i<amd->NumPolygons; i++) {

        void *next;

        struct rast_poly *rp = (struct rast_poly *) (msg->argstack+1);
        rp->flags  = pflags;
        rp->xyz    = (struct rast_xyz *) (rp+1); // unmittelbar nach rast_poly!
        rp->map[0] = txtbmp;
        sex.pnum   = amd->AttrsPool[i].poly_num;
        sex.poly   = rp;
        sex.shade  = ((FLOAT)amd->AttrsPool[i].shade_val)/256.0;
        if (amd->OutlinePool) sex.uv = amd->OutlinePool[i];
        else                  sex.uv = NULL;

        if (next = (void *) _methoda(msg->sklto, SKLM_EXTRACTPOLY, &sex)) {

            /*** Polygon ist sichtbar! ***/
            FLOAT max_z;
            ULONG j;

            /*** nur sichtbare ADEs zählen! ***/
            msg->ade_count++;

            /*** NoShade- und BlackShade-Optimierung ***/
            if (rp->flags & (RPF_GradShade|RPF_FlatShade)) {
                ULONG full_black = 0;
                ULONG full_white = 0;
                for (j=0; j<rp->pnum; j++) {
                    if (rp->b[j] < 0.01)      full_white++;
                    else if (rp->b[j] > 0.99) full_black++;
                };
                if (full_white == j) rp->flags &= ~(RPF_GradShade|RPF_FlatShade);
                else if (full_black == j) rp->flags = 0;
            };

            /** größtes Z finden ***/
            max_z = 0.0;
            for (j=0; j<rp->pnum; j++) {
                if (rp->xyz[j].z > max_z) max_z = rp->xyz[j].z;
            };

            /*** Publish-Stack-Element füllen ***/
            msg->pubstack->depth = FLOAT_TO_INT(max_z);
            msg->pubstack->args  = msg->argstack;
            msg->pubstack++;

            /*** Arg-Stack-Element auffüllen und weiterschalten ***/
            msg->argstack->draw_func = (void(*)(void *)) _DrawPolygon;
            msg->argstack = next;

        };
    };

    /*** Ende ***/
}

