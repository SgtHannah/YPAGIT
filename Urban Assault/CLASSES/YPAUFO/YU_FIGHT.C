/*
**  $Source: $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:24:47 $
**  $Locker: $
**  $Author: $
**
**  Die Ballerroutinen!!!!!!!!!!!!!!
**
**  (C) Copyright 1995 by Andreas Flemming
*/
#include <exec/types.h>

#include <math.h>

#include "nucleus/nucleus2.h"
#include "engine/engine.h"
#include "transform/tform.h"
#include "ypa/ypaworldclass.h"
#include "ypa/ypabactclass.h"
#include "ypa/ypamissileclass.h"
#include "input/input.h"
#include "ypa/ypaufoclass.h"


/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine


#define SHOT_DIST       500.0           // für MGs etc.
#define MISSILE_DIST    2000.0          // für Raketen

_dispatcher(void, yu_YBM_FIGHTBACT, struct fight_msg *fight)
{
/*
**  FUNCTION
**
**
**  INPUT
**
**
**  RESULTS
**
**
**  CHANGED
**
*/


}

_dispatcher(void, yu_YBM_FIGHTSECTOR, struct fight_msg *fight)
{
/*
**  FUNCTION
**
**      schießt mit Raketen, denn die Bordkanone bewirkt nix!
**
**
**  INPUT
**
**
**  RESULTS
**
**
**  CHANGED
**
**      29-Nov-95      created     8100000C
**
*/

    struct ypaufo_data *yud;
    struct flt_triple richtung;
    FLOAT distance;
    struct setstate_msg state;
    struct firemissile_msg fire;
    struct settarget_msg target;
    struct checklandpos_msg clp;
    LONG Energy;
    UBYTE aggr;

    yud = INST_DATA(cl, o );

    /*
    ** Es wird ein Sektor bekämpft. Dazu beschießen wir ihn, wenn sein Eigentümer 
    ** nicht unser Eigentümer ist. Andernfalls fliegen wir bloß hin und gehen 
    ** dort in den WAIT-Zustand über, was wir ja auch machen, wenn der Sektor
    ** uns überschrieben wurde.
    ** Durch die Zielreihenfolge werden auch die Eindringlinge auf eigenem
    ** Gebiet zuerst bekämpft!
    */


    /* Ballern oder nicht ? Entweder wir sind unter Aggr 100, dann genügt es
    ** zu wissen, ob der Sektor uns gehört. Bei Aggr 100 muß die Energie auf
    ** 0 gesunken sein. */

    if( yud->bact->Aggression >= 100 ) {

        if( fight->enemy.sector->SType == SECTYPE_COMPACT )
            Energy = fight->enemy.sector->SubEnergy[0][0];
        else {
            
            int i,j;
            Energy = 0;
            for( i=0; i<3; i++ ) for(j=0; j<3; j++ ) 
                 Energy += fight->enemy.sector->SubEnergy[i][j];
            }
        }
    else
        Energy = 1000;      /*** irgendwas eben ***/

    aggr = yud->bact->Aggression;

    /* Wir kämpfen nicht und landen:
    ** ( Wenn der Sektor uns gehört || Der Sektor niemanden gehört ) &&
    ** wenn wir nicht auf einem Slurp sind */

    /* Wenn wir unter 100% Argression sind, dann lohnt es sich nachzudenken, ob 
    ** wir das Ziel angreifen sollen. */

    if( (aggr < 100) || (Energy <= 0) ) {

        /* Es lohnt zu testen */
        if( (yud->bact->Owner == fight->enemy.sector->Owner) || 
            (fight->enemy.sector->Owner == 0) ) {

            /* Der Sektor gehört uns oder ist auf 0-Niveau gebomt worden.
            ** Somit gibt es erstmal keinen Kampf. Es lohnt aber noch zu testen, 
            ** ob wir schon über Gebiet sind, wo man landen kann */

            clp.flags = CLP_NOSLURP;
            if( (yud->bact->Sector == fight->enemy.sector) &&
                _methoda( o, YBM_CHECKLANDPOS, &clp) ) {

                /* Mir sei do! */
                state.extra_off = EXTRA_FIRE;
                state.extra_on  = 0;
                state.main_state= ACTION_WAIT;
                _methoda( o, YBM_SETSTATE, &state );

                if( yud->bact->SecondaryTarget.Sector == fight->enemy.sector ) {

                    /* Das HZ wird nicht ausgeschalten. */
                    target.priority    = 1;
                    target.target.bact = NULL;
                    target.target_type = TARTYPE_NONE;
                    _methoda( o, YBM_SETTARGET, &target);
                    }
                }
            return;
            }
        }



    /* Wir Korrigieren das Ziel auf den energiereichsten SubSektor */
    if( fight->enemy.sector == yud->bact->PrimaryTarget.Sector ) {
        _methoda( o, YBM_GETBESTSUB, &(yud->bact->PrimPos));
        fight->pos = yud->bact->PrimPos;
        }
    else {
        _methoda( o, YBM_GETBESTSUB, &(yud->bact->SecPos));
        fight->pos = yud->bact->SecPos;
        }

    /* für die Richtung vergleichen wir die Entfernung zum Sektormittel-
    ** punkt mit einer Konstanten und weiterhin testen wir, ob das Skalar-
    ** produkt aus lokal_z und Richtung zum MP positiv ist. Das geht 
    ** vielleicht. Denn wir bekämpfen etwas, was hoch ist, da geht die 
    ** Richtung zum MP nicht allein.
    */


    richtung.x = fight->pos.x - yud->bact->pos.x;
    richtung.z = fight->pos.z - yud->bact->pos.z;
    richtung.y = yud->bact->pref_height;             /* so ungefähr...*/

    distance = sqrt( richtung.x * richtung.x + richtung.z * richtung.z );
    
    richtung.x /= distance;
    richtung.y /= distance;
    richtung.z /= distance;


    /* Rakete abfeuern ? */
    if( distance < MISSILE_DIST ) {

        /* Wir übergeben den Raketen unser Ziel. (0.94 = 20 Grad) */

        if( richtung.x*yud->bact->dir.m31 +
            richtung.y*yud->bact->dir.m32 +
            richtung.z*yud->bact->dir.m33 > 0.94 ) {

            fire.dir.x         = yud->bact->dir.m31;
            fire.dir.y         = yud->bact->dir.m32 - 0.2;       // Hack!
            fire.dir.z         = yud->bact->dir.m33;
            fire.target_type   = TARTYPE_SECTOR;
            fire.target.sector = fight->enemy.sector;
            fire.target_pos    = fight->pos;
            fire.wtype         = yud->bact->auto_ID;
            fire.global_time   = fight->global_time;
            /* Hubschrauber. etwas runter + Seite */
            if( fight->global_time % 2 == 0 )
                fire.start.x   = -15;
            else
                fire.start.x   = 15;
            fire.start.y       = 5;
            fire.start.z       = 0;
            _methoda( o, YBM_FIREMISSILE, &fire );
            return;     /* wenn rakete, dann nicht Kanone */
            }
        }

}




