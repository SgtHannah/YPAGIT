/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_deathcache.c,v $
**  $Revision: 38.14 $
**  $Date: 1998/01/06 16:17:32 $
**  $Locker:  $
**  $Author: floh $
**
**  DeathCache-Handling.
**
**  (C) Copyright 1995 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/lists.h>

#include "nucleus/nucleus2.h"
#include "engine/engine.h"

#include "ypa/ypabactclass.h"
#include "ypa/ypaworldclass.h"
#include "ypa/ypaflyerclass.h"
#include "ypa/ypatankclass.h"
#include "ypa/ypamissileclass.h"

#include "yw_protos.h"

_extern_use_nucleus
_extern_use_audio_engine

extern ULONG DeathCount;

/*-----------------------------------------------------------------*/
UBYTE *ClassIDStrings[] = {
    "dummy.class",          // BCLID_NOCLASS
    "ypabact.class",        // BCLID_YPABACT
    "ypatank.class",        // BCLID_YPATANK
    "yparobo.class",        // BCLID_YPAROBO
    "ypamissile.class",     // BCLID_YPAMISSY
    "ypazepp.class",        // BCLID_YPAZEPP
    "ypaflyer.class",       // BCLID_YPAFLYER
    "ypaufo.class",         // BCLID_YPAUFO
    "ypacar.class",         // BCLID_YPACAR
    "ypagun.class",         // BCLID_YPAGUN
    "ypahovercraft.class",  // BCLID_YPAHOVERCRAFT
};

/*-----------------------------------------------------------------*/
_dispatcher(Object *, yw_YWM_OBTAINVEHICLE, struct obtainvehicle_msg *msg)
/*
**  FUNCTION
**      siehe Spezifikation in "ypa/ypaworldclass.h"
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      23-Sep-95   floh    created
**      09-Oct-95   floh    + AutoCount eliminiert
**                          + Ooops, air_const wurde nicht initialisiert
**      10-Oct-95   8100 000C  Weltgröße eintragen
**      11-Oct-95   floh    jedes neue Objekt bekommt einen
**                          garantiert uniquen Identifier
**      24-Oct-95   floh    AF's Änderungen übernommen, alle Energien
**                          jetzt auf Watt, statt milliWatt,
**                          außerdem wird bact.Maximum initialisiert
**                          (etc...)
**      29-Oct-95   floh    update
**      21-Nov-95   floh    Änderung von AF übernommen
**      22-Nov-95   floh    Änderungen von AF übernommen (Reincarnate!)
**      09-Feb-96   floh    unschädlich gemacht
*/
{
    return(NULL);
}

/*-----------------------------------------------------------------*/
Object *yw_PrivObtainBact(Object *o,
                          struct ypaworld_data *ywd,
                          ULONG bclid)
/*
**  FUNCTION
**      Private Version von Obtain-Vehicle, die von
**      YWM_CREATEWEAPON und YWM_CREATEVEHICLE benutzt
**      wird. Scannt den DeathCache nach einem geeigneten
**      Object oder erzeugt selbständig ein neues
**      (also eine Teilfunktionalität von YWM_OBTAINVEHICLE).
**
**  INPUTS
**      o       -> Ptr auf Welt-Object
**      ywd     -> Ptr auf LID des Welt-Objects
**      bclid   -> die herkömmliche BactClassID des
**                 gewünschten Objects.
**
**  RESULTS
**      Object *, oder NULL falls was nicht geklappt hat
**
**  CHANGED
**      06-Dec-95   floh    created
**      17-Dec-95   8100 000C Weltgröße jetzt in REINCARNATE und MAIN
**      10-Nov-96   floh    Owner=0
*/
{
    struct MinList *ls;
    struct MinNode *nd;

    struct Bacterium *v_bact;
    Object *v_obj = NULL;

    /*** DeathCache scannen ***/
    ls = &(ywd->DeathCache);
    for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {

        v_bact = ((struct OBNode *)nd)->bact;

        if (v_bact->BactClassID == bclid) {
            /*** Treffer ***/
            v_obj = v_bact->BactObject;
            break;
        };
    };

    /*** falls im TotenCache nix gefunden, Gott spielen... ***/
    if (v_obj == NULL) {

        v_obj = _new(ClassIDStrings[bclid], YBA_World, o, TAG_DONE);
        if (v_obj) {
            _get(v_obj, YBA_Bacterium, &v_bact);
        } else return(NULL);
    };

    _methoda( v_bact->BactObject, YBM_REINCARNATE, NULL );

    v_bact->ident = DeathCount++;
    v_bact->Owner = 0;

    v_bact->dir.m11 = 1.0;
    v_bact->dir.m12 = 0.0;
    v_bact->dir.m13 = 0.0;
    
    v_bact->dir.m21 = 0.0;
    v_bact->dir.m22 = 1.0;
    v_bact->dir.m23 = 0.0;
    
    v_bact->dir.m31 = 0.0;
    v_bact->dir.m32 = 0.0;
    v_bact->dir.m33 = 1.0;

    /*** Ende ***/
    return(v_obj);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, yw_YWM_RELEASEVEHICLE, Object *v_obj)
/*
**  FUNCTION
**      siehe Spezifikation in "ypa/ypaworldclass.h"
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      23-Sep-95   floh    created
**      08-Jan-96   floh    Objekte werden jetzt generell auf den
**                          Vehikel-Schrottplatz in Sektor 0,0
**                          gesetzt.
**      11-Mar-96   floh    AF's DeatchCache-Deadlock Bug-Check
**                          übernommen...
**      26-Apr-96   floh    _KillSoundCarrier() auf SoundCarrier-Struct
**                          im Bakterium.
**      03-May-97   floh    + tote Vehikel werden nicht auf 50/50,
**                            sondern auf Original-Pos + nach
**                            unten verschoben.
**      27-May-97   floh    + Umplazierung toter Vehikel wieder mit
**                            YBM_SETPOSITION, aber mit YBFSP_DontCorrect
**                            gesetzt
**      29-May-97   floh    + SetPosition war Fucked up, weil als x/z-
**                            Position die originale Position des Objekts
**                            genommen wurde
*/
{
    struct ypaworld_data *ywd = INST_DATA(cl,o);
    struct newmaster_msg kiss_of_death;
    struct setposition_msg pos;
    struct Bacterium *b;

    _get(v_obj, YBA_Bacterium, &b);

    /*** AF's Debugging-Check FIXME ***/
    if(b->BactClassID == BCLID_YPAMISSY) {
        if(b->PrimTargetType != 0) {
            _LogMsg("OH NO! The DEATH CACHE BUG is back!\n");
        };
    };

    /*** Sounds abwürgen ***/
    _KillSoundCarrier(&(b->sc));

    /*** einfach eine YBM_NEWMASTER-Methode auf das Objekt ***/
    kiss_of_death.master      = (void *) 1L;    // != 0
    kiss_of_death.master_bact = NULL;
    kiss_of_death.slave_list  = &(ywd->DeathCache);
    _methoda(v_obj, YBM_NEWMASTER, &kiss_of_death);

    /*** Objekt umsetzen ***/
    pos.x = 600;
    pos.y = -50000.0;
    pos.z = -600;
    pos.flags = YBFSP_DontCorrect;
    _methoda(v_obj, YBM_SETPOSITION, &pos);

    /*** Unsichtbar ***/
    b->ExtraState |= EXTRA_DONTRENDER;
}



