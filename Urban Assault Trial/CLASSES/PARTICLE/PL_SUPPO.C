/*
**  $Source: PRG:VFM/Classes/_ParticleClass/pl_support.c,v $
**  $Revision: 38.10 $
**  $Date: 1996/09/22 02:33:06 $
**  $Locker:  $
**  $Author: floh $
**
**  pl_support.c -- Support-Routinen zum Verwalten der Particle-Spaces.
**
**  (C) Copyright 1995 by A.Weissflog
*/
#include <exec/types.h>

#include <math.h>

#include "nucleus/nucleus2.h"
#include "engine/engine.h"
#include "transform/te.h"
#include "transform/tform.h"
#include "ade/particleclass.h"

/*** aus pl_rand.c ***/
extern FLOAT prtl_Rand(void);

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine


/*-----------------------------------------------------------------*/
struct Context *prtl_FindContext(struct particle_data *pd, ULONG id)
/*
**  FUNCTION
**      Ermittelt einen Pointer auf den per <id> definierten
**      Context. Falls kein Kontext mit dieser ID existiert,
**      wird NULL zurückgegeben.
**
**  INPUTS
**      pd  -> Ptr auf LID des particle.class Objects
**      id  -> eine 32 Bit ID, != 0
**
**  RESULTS
**      Ptr auf Context-Struktur, oder NULL, falls kein Context
**      mit dieser ID existiert.
**
**  CHANGED
**      11-Sep-95   floh    created
**      17-Jan-96   floh    revised & updated
*/
{
    struct Context *ctx = pd->CtxStart;
    do {
        if (id == ctx->Id) return(ctx);
    } while (++ctx != pd->CtxEnd);
    return(NULL);
}

/*-----------------------------------------------------------------*/
struct Context *prtl_AllocContext(struct particle_data *pd,
                                  ULONG id, LONG timestamp)
/*
**  FUNCTION
**      Allokiert den nächsten Kontext im Ringbuffer.
**      Falls an der nächsten Position ein Kontext
**      existiert, der noch in Arbeit ist, wird er gekillt,
**      wenn er seit mehr als 1 Sekunde existiert, andernseits
**      kommt die Routine NULL zurück.
**
**  INPUTS
**      pd  -> zeigt auf LID des Objects
**      id  -> 32-Bit-ID für neuen Kontext
**      timestamp -> aktueller Timestamp
**
**  RESULTS
**
**  CHANGED
**      11-Sep-95   floh    created
**      17-Jan-96   floh    revised & updated
*/
{
    struct Context *ctx = pd->CtxNext;

    if (ctx->Id != 0) {
        /*** dieser ist besetzt, wahres Alter weniger als 1 Sek? ***/
        LONG age = timestamp - ctx->Timestamp;
        if (age < 1000) return(NULL);   // bleibt besetzt
    };

    /*** sonst allokieren ***/
    ctx->Id = id;
    pd->CtxNext++;
    if (pd->CtxNext == pd->CtxEnd) pd->CtxNext=pd->CtxStart;
    return(ctx);
}

/*-----------------------------------------------------------------*/
void prtl_InitContext(struct particle_data *pd, 
                      struct Context *ctx,
                      ULONG timestamp)
/*
**  FUNCTION
**      Initialisiert einen mit prtl_AllocContext() allokierten
**      Context als neuen Particle-Space.
**
**  INPUTS
**      pd  -> Ptr auf LID des Objects
**      ctx -> Ptr auf bereits allokierten Context
**      timestamp   -> aktuelle GlobalTime aus <publish_msg>
**
**  RESULTS
**      ---
**
**  CHANGED
**      11-Sep-95   floh    created
**      14-Sep-95   floh    neu: ctx->OldestAge wird initialisiert
**      17-Jan-96   floh    revised & updated
*/
{
    ctx->Oldest    = ctx->Start;
    ctx->Latest    = ctx->Start;
    ctx->OldestAge = 0;
    ctx->Timer     = 0;
    ctx->Timestamp = timestamp;
    ctx->Age       = 0;
}

/*-----------------------------------------------------------------*/
void prtl_KillParticle(struct particle_data *pd,
                       struct Context *ctx)
/*
**  FUNCTION
**      Killt das älteste Partikel im Context. Muß nur verwendet
**      werden, wenn der Context keine neuen Partikel mehr
**      erzeugt, weil prtl_CreateParticle() selbst für
**      das korrekte Beseitigen nicht mehr existenter
**      Partikel sorgt.
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      12-Sep-95   floh    created
**      14-Sep-95   floh    neu: ctx->OldestAge -= pd->Threshold
**      17-Jan-96   floh    revised & updated
*/
{
    /*** nur, wenn Puffer nicht leer ist ***/
    ctx->OldestAge -= pd->Threshold;
    if (ctx->Latest != ctx->Oldest) {
        ctx->Oldest++;
        if (ctx->Oldest == ctx->End) ctx->Oldest = ctx->Start;
    };
}

/*-----------------------------------------------------------------*/
void prtl_CreateParticle(struct particle_data *pd,
                         struct Context *ctx,
                         struct ptc_vec *mag,
                         struct ptc_vec *org,
                         FLOAT t)
/*
**  FUNCTION
**      Erzeugt im angebenen Context ein neues Particle,
**      und gibt diesem eine Anfangs-Position und -Geschwindigkeit.
**
**      Falls der Partikel-Puffer voll ist, stirbt mit Erschaffen
**      eines neuen Partikels gleichzeitig das bis dahin älteste.
**      Damit muß prtl_CreateParticles() nur noch mit der
**      richtigen "Frequenz" aufgerufen werden.
**
**      Bei jedem Aufruf wird von ctx->Timer der Wert pd->Threshold
**      subtrahiert. Ein normaler "Particle-Frame" sieht also
**      so aus:
**
**          ctx->Timer += FrameTime
**          while (ctx->Timer >= 0) {
**              prtl_CreateParticle(pd,ctx);
**          };
**
**      Das Alter des jüngsten Partikels entspricht
**      (ctx->Timer + ctx->Threshold) millisec, jedes folgende Partikel ist
**      pd->Threshold millisec älter.
**
**  INPUTS
**      pd  -> Pointer auf LID des Objects
**      ctx -> Pointer auf gültigen Context
**      mag -> Pointer auf Magnifier-Vector (Global-Space)
**      org -> Pointer auf Ursprungspunkt des Partikels (Global-Space)
**      t   -> relative Subframe-Time
**
**  RESULTS
**      ---
**
**  CHANGED
**      11-Sep-95   floh    created
**      14-Sep-95   floh    neues Partikel: ctx->OldestAge += pd->Threshold
**                          Partikel stirbt: ctx->OldestAge -= pd->Threshold
**      05-Oct-95   floh    ctx->OldestAge wird nur noch beeinflußt, wenn
**                          ein Partikel stirbt
**      17-Jan-96   floh    revised & updated
**      02-Sep-96   floh    + "Subframe-Positionierung"
*/
{
    struct Particle *p;
    FLOAT vx,vy,vz,w;

    /*** neues Partikel + Ringbuffer-Pointer updaten ***/
    p = ctx->Latest++;
    if (ctx->Latest == ctx->End)  ctx->Latest = ctx->Start;

    /*** Buffer voll? Dann geht automatisch eins drauf. ***/
    if (ctx->Latest == ctx->Oldest) {
        ctx->OldestAge -= pd->Threshold;
        ctx->Oldest++;
        if (ctx->Oldest == ctx->End) ctx->Oldest = ctx->Start;
    };

    /*** Anfangs-Richtung des Partikels ***/
    vx = prtl_Rand() + mag->x;
    vy = prtl_Rand() + mag->y;
    vz = prtl_Rand() + mag->z;

    /*** Richtungs-Vektor normalisieren und mit StartSpeed mult. ***/
    w = sqrt(vx*vx + vy*vy + vz*vz);
    p->vx = vx/w * pd->StartSpeed;
    p->vy = vy/w * pd->StartSpeed;
    p->vz = vz/w * pd->StartSpeed;

    /*** Anfangs-Position des Partikels im Global-Space ***/
    p->px = org->x + p->vx*t;
    p->py = org->y + p->vy*t;
    p->pz = org->z + p->vz*t;

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void prtl_PublishParticle(struct particle_data *pd,
                          struct Particle *p,
                          struct publish_msg *msg,
                          FLOAT size,
                          ULONG age)
/*
**  FUNCTION
**      "Positioniert" das eingebettete ADE auf das übergebene
**      Particle und publisht dieses auf die normale A&W.
**
**  INPUTS
**      pd       -> Pointer auf LID des Particle-Objects
**      particle -> das zu publishende Particle
**      msg      -> die komplette ADE-publish_msg.
**      size     -> aktuelle Größe eines Partikels
**      age      -> Alter des Partikels in millisec
**
**  RESULTS
**      ---
**
**  CHANGED
**      12-Sep-95   floh    created
**      16-Sep-95   floh    jetzt mit veränderter Ausrichtung
**      04-Oct-95   floh    sucht sich jetzt dem Alter entsprechend
**                          das richtige ADE raus.
**      14-Oct-95   floh    Partikel werden jetzt korrekt zentriert
**      12-Nov-95   floh    Auswahl des Partikel-ADEs jetzt sicherer.
**                          In manchen Fällen hat <age> einen sehr
**                          großen Wert, der daraufhin zu Enforcer Hits
**                          führte.
**      17-Jan-96   floh    revised & updated (ok, ich weiß die Sache
**                          ist nach wie vor etwas umständlich, um eine
**                          rechteckige Bitmap auf den Screen zu bringen)
**      29-Apr-96   floh    Bugfix: jetzt dürften die Partikel endlich
**                          IMMER korrekt zentriert sein.
**      11-Jun-96   floh    Detail-Änderung im ADE-Publishing-Part
**                          (Object *sklto in publish_msg)
*/
{
    ULONG i;
    ULONG and_code;
    FLOAT tlx,tly,tlz;
    fp3d *pool = pd->Sklt->TransformPool;

    /*** aktuelle Rot-Matrix des Viewers ***/
    struct flt_m3x3 *vm = &(msg->viewer->glb_m);

    /*** [tlx,tly,tlz] entspricht linke obere Ecke ***/
    tlx = p->px - ((vm->m11+vm->m21) * size/2.0);
    tly = p->py - ((vm->m12+vm->m22) * size/2.0);
    tlz = p->pz - ((vm->m13+vm->m23) * size/2.0);

    /*** Koordinaten von Hand generieren ***/
    and_code = 0xffffffff;
    for (i=0; i<5; i++) {

        FLOAT gx,gy,gz;
        FLOAT vx,vy,vz;
        ULONG cc;

        /*** Global-Koordinaten ***/
        switch(i) {
            case 0: // Mittelpunkt
                gx = p->px;
                gy = p->py;
                gz = p->pz;
                break;
            case 1: // linke obere Ecke
                gx = tlx;
                gy = tly;
                gz = tlz;
                break;
            case 2: // rechte obere Ecke
                gx = tlx + vm->m11 * size;
                gy = tly + vm->m12 * size;
                gz = tlz + vm->m13 * size;
                break;
            case 3: // rechte untere Ecke
                gx = tlx + ((vm->m11 + vm->m21) * size);
                gy = tly + ((vm->m12 + vm->m22) * size);
                gz = tlz + ((vm->m13 + vm->m23) * size);
                break;
            case 4: // linke untere Ecke
                gx = tlx + vm->m21 * size;
                gy = tly + vm->m22 * size;
                gz = tlz + vm->m23 * size;
                break;
        };

        /*** Global-2-Viewer Transformation ***/
        gx -= msg->viewer->glb.x;
        gy -= msg->viewer->glb.y;
        gz -= msg->viewer->glb.z;

        vx = vm->m11*gx + vm->m12*gy + vm->m13*gz;
        vy = vm->m21*gx + vm->m22*gy + vm->m23*gz;
        vz = vm->m31*gx + vm->m32*gy + vm->m33*gz;

        /*** ClipCodes bestimmen ***/
        cc = 0;
        if      (vz < msg->min_z) cc |= CLIP3D_BEHIND;
        else if (vz > msg->max_z) cc |= CLIP3D_TOOFAR;
        if (vx > vz)  cc |= CLIP3D_RIGHTOUT;
        if (vx < -vz) cc |= CLIP3D_LEFTOUT;
        if (vy > vz)  cc |= CLIP3D_BOTOUT;
        if (vy < -vz) cc |= CLIP3D_TOPOUT;
        and_code &= cc;

        pool[i].flags = cc;
        pool[i].x     = vx;
        pool[i].y     = vy;
        pool[i].z     = vz;
    };

    /*** ist das Teil überhaupt sichtbar? ***/
    if (and_code == 0) {

        struct Skeleton *old_sklt;
        Object *old_sklto;

        /*** übergeordnetes Skeleton temporär patchen ***/
        old_sklt   = msg->sklt;
        old_sklto  = msg->sklto;
        msg->sklt  = pd->Sklt;
        msg->sklto = pd->Skeleton;

        /*** das "richtige" ADE publishen ***/
        if (pd->ADETimeDiff != 0) {

            ULONG ade_num = age / pd->ADETimeDiff;

            if (ade_num < PTL_MAXNUMADES) {
                Object *ade = pd->ADEArray[ade_num];
                if (ade) {
                    _methoda(ade, ADEM_PUBLISH, msg);
                };
            };
        };

        /*** alte msg wiederherstellen ***/
        msg->sklt  = old_sklt;
        msg->sklto = old_sklto;
    };
}

/*-----------------------------------------------------------------*/
void prtl_DoContext(struct particle_data *pd,
                    struct Context *ctx,
                    struct publish_msg *msg)
/*
**  FUNCTION
**      Updated Particle-Space des angegebenen Contexts.
**      Das beinhaltet die Erschaffung neuer und
**      die Bewegung der existierenden Partikel.
**
**  INPUTS
**      pd  -> Pointer auf LID des Objects
**      ctx -> Pointer auf Context
**      msg -> Pointer auf vollständige ADE-publish_msg.
**
**  RESULTS
**      ---
**
**  CHANGED
**      11-Sep-95   floh    created
**      13-Sep-95   floh    (1) Particle wurden in falscher
**                              Reihenfolge abgehandelt.
**                          (2) Zeitpunkte für Start und Stop der
**                              Partikel-Erzeugung sind jetzt per
**                              PTLA_StartGeneration und PTLA_StopGeneration
**                              einstellbar.
**      05-Oct-95   floh    more debugging (da war noch ein kleiner
**                          Denkfehler in Bezug auf ctx->OldestAge
**      17-Jan-96   floh    revised & updated
**      02-Sep-96   floh    neue Partikel werden jetzt "subframe-positioniert"
*/
{
    FLOAT t;
    FLOAT flt_age;

    /*** Context-Timer updaten ***/
    ctx->Timer     += msg->frame_time;
    ctx->OldestAge += msg->frame_time;

    /*** FrameTime in sec(!) ***/
    t = ((FLOAT)(msg->frame_time))/1000;

    /*** Kontext-Alter updaten ***/
    ctx->Age = msg->time_stamp - ctx->Timestamp;
    flt_age = (FLOAT) ctx->Age;

    /*** Kontext tot? ***/
    if (ctx->Age >= pd->CtxLifetime) {

        ctx->Id = 0;
        return;

    } else if (ctx->Age >= pd->CtxStopGen) {

        /* Partikel-Produktion hat aufgehört */
        if (ctx->Latest != ctx->Oldest) {

            /* es müssen evtl. Partikel gekillt werden */
            while (ctx->OldestAge >= pd->Lifetime) {
                prtl_KillParticle(pd,ctx);
            };

        } else {
            /* es sind schon alle tot */
            return;
        };

    } else if (ctx->Age >= pd->CtxStartGen) {

        /*** neue Partikel generieren, falls die Zeit reif ist... ***/
        if (ctx->Timer >= 0) {

            struct ptc_vec vec;
            struct ptc_vec org;
            fp3d *pnt;
            FLOAT tx,ty,tz;
            struct flt_m3x3 *m = &(msg->owner->glb_m);
            FLOAT rel_t  = 0.0;
            FLOAT rel_dt = ((FLOAT)pd->Threshold)/1000.0;

            tx = pd->StartMagnify.x + (pd->AddMagnify.x * flt_age);
            ty = pd->StartMagnify.y + (pd->AddMagnify.y * flt_age);
            tz = pd->StartMagnify.z + (pd->AddMagnify.z * flt_age);

            /*** diesen Vektor jetzt noch rotieren ***/
            vec.x = m->m11*tx + m->m12*ty + m->m13*tz;
            vec.y = m->m21*tx + m->m22*ty + m->m23*tz;
            vec.z = m->m31*tx + m->m32*ty + m->m33*tz;

            /*** Ursprungs-Punkt des Partikels im Object-System ***/
            pnt = &(msg->sklt->Pool[pd->PointNum]);
            tx  = pnt->x;
            ty  = pnt->y;
            tz  = pnt->z;

            /*** ...und im Global-System ***/
            org.x = (m->m11*tx + m->m12*ty + m->m13*tz) + msg->owner->glb.x;
            org.y = (m->m21*tx + m->m22*ty + m->m23*tz) + msg->owner->glb.y;
            org.z = (m->m31*tx + m->m32*ty + m->m33*tz) + msg->owner->glb.z;

            /* n Partikel erzeugen */
            while (ctx->Timer >= 0) {
                prtl_CreateParticle(pd,ctx,&vec,&org,rel_t);
                ctx->Timer -= pd->Threshold;
                rel_t += rel_dt;
            };
        };
    };

    /*** falls Partikel existieren, diese bewegen ***/
    if (ctx->Latest != ctx->Oldest) {

        FLOAT act_age, diff_age;
        ULONG act_int_age;
        struct ptc_vec speed_diff;
        struct Particle *actp;

        /*** aktuelle Geschwindigkeits-Änderung ***/
        speed_diff.x = pd->StartAccel.x + (pd->AddAccel.x * flt_age);
        speed_diff.y = pd->StartAccel.y + (pd->AddAccel.y * flt_age);
        speed_diff.z = pd->StartAccel.z + (pd->AddAccel.z * flt_age);

        act_age     = ctx->OldestAge;          // Alter des ältesten Partikels
        act_int_age = (ULONG) act_age;
        diff_age    = (FLOAT) -pd->Threshold;  // Altersdifferenz

        /* das momentan älteste Partikel */
        actp = ctx->Oldest;
        while (actp != ctx->Latest) {

            FLOAT act_size;

            /*** aktuelle Partikel-Parameter ***/
            act_size = pd->StartSize + (act_age*pd->AddSize);

            actp->vx += speed_diff.x;
            actp->vy += speed_diff.y;
            actp->vz += speed_diff.z;

            /*** Positions-Änderung (mit Noise!) ***/
            actp->px += actp->vx*t + (prtl_Rand()*pd->Noise);
            actp->py += actp->vy*t + (prtl_Rand()*pd->Noise);
            actp->pz += actp->vz*t + (prtl_Rand()*pd->Noise);

            /*** aktuelles Partikel publishen ***/
            prtl_PublishParticle(pd,actp,msg,act_size,act_int_age);

            act_age     += diff_age;
            act_int_age -= pd->Threshold;
            actp++;
            if (actp == ctx->End) actp = ctx->Start;
        };
    };
}

/*-----------------------------------------------------------------*/
_dispatcher(void, prtl_ADEM_PUBLISH, struct publish_msg *msg)
/*
**  FUNCTION
**      ADEM_PUBLISH-Dispatcher der particle.class.
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      12-Sep-95   floh    created
**      17-Jan-96   floh    revised & updated
*/
{
    struct particle_data *pd = INST_DATA(cl,o);
    struct Context *ctx;

    /*** dieser Kontext bereits in Arbeit? ***/
    ctx = prtl_FindContext(pd,msg->owner_id);
    if (NULL == ctx) {
        /*** nein, zusehen, daß wir einen neuen kriegen ***/
        ctx = prtl_AllocContext(pd,msg->owner_id,msg->time_stamp);
        if (ctx) prtl_InitContext(pd,ctx,msg->time_stamp);
        else return;
    };

    /*** Kontext abhandeln ***/
    prtl_DoContext(pd, ctx, msg);

    /*** Ende ***/
}
