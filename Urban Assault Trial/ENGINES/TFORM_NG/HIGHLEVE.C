/*
**  $Source: PRG:VFM/Engines/TransEngineNG/highlevel.c,v $
**  $Revision: 38.18 $
**  $Date: 1996/06/16 01:59:37 $
**  $Locker:  $
**  $Author: floh $
**
**  Highlevel-Routinen für ANSI-Transformer-Engine.
**
**  (C) Copyright 1994 by A.Weissflog
*/
/* Amiga-[Emul]-Includes */
#include <exec/types.h>

#include <math.h>

/* VFM-Includes */
#include "nucleus/nucleus2.h"
#include "engine/engine.h"
#include "transform/te_ng.h"

/*-------------------------------------------------------------------
** externe Referenzen
*/
extern struct TE_Base TEBase;

extern ULONG te_SetClipCode(fp3d *);
extern void  te_GetMatrix(tform *);

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 void __asm te_SetViewer(__a0 tform *viewer)
#else
void te_SetViewer(tform *viewer)
#endif
/*
**  FUNCTION
**      Definiert den ab jetzt gültigen Beobachter.
**
**  INPUTS
**      viewer  -> voll ausgefüllte TForm, die das Beobachter-
**                 Koordinaten-System vollständig definiert.
**
**  RESULTS
**
**  CHANGED
**      07-Nov-94   floh    created
**      02-Jan-94   floh    jetzt Nucleus2-kompatibel
*/
{
    /* Viewer-Orientation-Pointer nach TEBase */
    TEBase.Viewer = viewer;
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 tform *__asm te_GetViewer(void)
#else
tform *te_GetViewer(void)
#endif
/*
**  FUNCTION
**      Gibt Pointer auf TForm des aktuellen Beobachters
**      zurück.
**
**  INPUTS
**
**  RESULTS
**      Pointer auf <tform> des aktuellen Beobachters.
**
**  CHANGED
**      07-Nov-94   floh    created
**      02-Jan-94   floh    jetzt Nucleus2-kompatibel
*/
{
    return(TEBase.Viewer);
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 void __asm te_RefreshTForm(__a0 tform *tf)
#else
void te_RefreshTForm(tform *tf)
#endif
/*
**  FUNCTION
**      Diese Routine muß immer aufgerufen werden, falls
**      eine oder mehrere der folgenden Sachen an einer
**      <tform>-Struktur von Hand geändert werden:
**
**          (1) die Achswinkel [ax,ay,az]
**
**      Die Routine bringt damit interne Parameter der tform,
**      die nicht zum direkten Zugriff freigegeben sind,
**      auf den neuesten Stand.
**
**  INPUTS
**      tf  -> Pointer auf TForm, die aktualisiert werden soll.
**
**  RESULTS
**      ---
**
**  CHANGED
**      07-Nov-94   floh    created
**      02-Jan-94   floh    jetzt Nucleus2-kompatibel
*/
{
    /* erzeuge die Rotations-Matrix neu */
    te_GetMatrix(tf);

    /* das war vorläufig alles */
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 void __asm te_TFormToGlobal(__a0 tform *tf)
#else
void te_TFormToGlobal(tform *tf)
#endif
/*
**  FUNCTION
**      Ermittelt die Position einer TForm im globalen
**      System, sowie die globale Rotations-/Skalierungs-Matrix.
**      Voraussetzung ist, daß falls die TForm eine Mother hat,
**      deren globale Position und Rot/Scale-Matrix bereits
**      auf dem neuesten Stand ist.
**      Falls die TForm keine Mother hat, werden naturgemäß
**      direkt die lokale Position und Matrix übernommen.
**      ACHTUNG: Außer einer vorhandenen Mother muß außerdem
**      das Flag TFF_FollowMother in der TForm gesetzt sein,
**      sonst wird angenommen, die TForm ist im Globalsystem
**      definiert!
**
**  INPUTS
**      Pointer auf zu globalisierende TForm
**
**  RESULTS
**      [gx,gy,gz]
**      [gm11..gm33]
**
**  CHANGED
**      24-Nov-94   floh    created
**      03-Dec-94   floh    angepaßt an neues TForm-Format
**      02-Jan-94   floh    jetzt Nucleus2-kompatibel
*/
{
    register tform *m;
    if ((m = tf->mother) && (tf->flags & TFF_FollowMother)) {

        /* ermittle globale Position... */
        FLOAT lx = tf->loc.x;
        FLOAT ly = tf->loc.y;
        FLOAT lz = tf->loc.z;
        tf->glb.x = (m->glb_m.m11*lx + m->glb_m.m12*ly + m->glb_m.m13*lz) + m->glb.x;
        tf->glb.y = (m->glb_m.m21*lx + m->glb_m.m22*ly + m->glb_m.m23*lz) + m->glb.y;
        tf->glb.z = (m->glb_m.m31*lx + m->glb_m.m32*ly + m->glb_m.m33*lz) + m->glb.z;

        /* ermittle globale Rotations/Skalierungs-Matrix... */
        /* also: GM = [M->GM] · [M] */
        tf->glb_m.m11 = m->glb_m.m11*tf->loc_m.m11 + m->glb_m.m12*tf->loc_m.m21 + m->glb_m.m13*tf->loc_m.m31;
        tf->glb_m.m12 = m->glb_m.m11*tf->loc_m.m12 + m->glb_m.m12*tf->loc_m.m22 + m->glb_m.m13*tf->loc_m.m32;
        tf->glb_m.m13 = m->glb_m.m11*tf->loc_m.m13 + m->glb_m.m12*tf->loc_m.m23 + m->glb_m.m13*tf->loc_m.m33;

        tf->glb_m.m21 = m->glb_m.m21*tf->loc_m.m11 + m->glb_m.m22*tf->loc_m.m21 + m->glb_m.m23*tf->loc_m.m31;
        tf->glb_m.m22 = m->glb_m.m21*tf->loc_m.m12 + m->glb_m.m22*tf->loc_m.m22 + m->glb_m.m23*tf->loc_m.m32;
        tf->glb_m.m23 = m->glb_m.m21*tf->loc_m.m13 + m->glb_m.m22*tf->loc_m.m23 + m->glb_m.m23*tf->loc_m.m33;

        tf->glb_m.m31 = m->glb_m.m31*tf->loc_m.m11 + m->glb_m.m32*tf->loc_m.m21 + m->glb_m.m33*tf->loc_m.m31;
        tf->glb_m.m32 = m->glb_m.m31*tf->loc_m.m12 + m->glb_m.m32*tf->loc_m.m22 + m->glb_m.m33*tf->loc_m.m32;
        tf->glb_m.m33 = m->glb_m.m31*tf->loc_m.m13 + m->glb_m.m32*tf->loc_m.m23 + m->glb_m.m33*tf->loc_m.m33;

    } else {

        tf->glb   = tf->loc;
        tf->glb_m = tf->loc_m;
    };

    /* Ende */
}

/*-- ENDE ---------------------------------------------------------*/
