/*
**  $Source: $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:24:47 $
**  $Locker: $
**  $Author: $
**
**  Die Intelligenzen...
**
**  (C) Copyright 1995 by Andreas Flemming
*/
#include <exec/types.h>

#include <math.h>

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "engine/engine.h"
#include "transform/tform.h"
#include "ypa/ypaworldclass.h"
#include "ypa/ypabactclass.h"
#include "ypa/ypagunclass.h"
#include "ypa/ypamissileclass.h"
#include "input/input.h"


/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine

void yg_vec_round_vec( struct flt_triple *rot, struct flt_triple *vec, FLOAT angle);
void yg_dir_round_vec( struct flt_triple *rot, struct flt_m3x3 *dir, FLOAT angle );



_dispatcher( BOOL, yg_YBM_SETPOSITION, struct setposition_msg *pos )
{
/*
**  FUNCTION    Überlagerung der Setposition. Nachträglich wird y so
**              korrigiert, daß neues y = Sektorhöhe + sp->x.
**              Das geht, weil in der Supermethode der Sektor richtig
**              gesetzt sein müßte.
**
**  INPUT       na setpos eben
**
**  RESULT      TRUE, wenn das ok war
**
**  CHANGED     created 20-Jan-96   8100 000C
**              changed 27-Jan-96   8100 000C o-pos nachgetragen
**              changed 27-Feb-96   8100 000C Childbehandlung
*/

    struct ypagun_data *ygd;
    struct intersect_msg inter;
    struct installgun_msg ig;
    struct handlevischildren_msg hvc;


    ygd = INST_DATA( cl, o );

    if( _supermethoda( cl, o, YBM_SETPOSITION, pos) ) {

        /*** Das war erstmal ok ***/
        if( ygd->flags & GUN_SetGround ) {

            /*** eine Intersection machen ***/
            inter.pnt.x = pos->x;
            inter.pnt.y = pos->y - 10000;
            inter.pnt.z = pos->z;
            inter.vec.x = 0.0;
            inter.vec.y = 20000.0;
            inter.vec.z = 0.0;
            inter.flags = 0;
            _methoda( ygd->world, YWM_INTERSECT, &inter );
            if( inter.insect ) {

                ygd->bact->pos.y = inter.ipnt.y - ygd->bact->over_eof;
                }

            /*** Install Gun ist von anderer Seite nicht zu erwarten ***/
            ig.basis.x = 0.0;
            ig.basis.y = 0.0;
            ig.basis.z = 1.0;
            ig.flags   = 0;
            _methoda( o, YGM_INSTALLGUN, &ig );
            }
        else {

            /*** Also nicht aufsetzen, aber vielleich an der Sektorhöhe anpassen? ***/
            if( !(pos->flags & YGFSP_SetFree ) )
                ygd->bact->pos.y = pos->y + ygd->bact->Sector->Height;
            }

        ygd->bact->old_pos = ygd->bact->pos;

        /*** jetzt ist alles getan und ich kopple die KInder ab ***/
        hvc.job = HVC_REMOVECHILDREN;
        _methoda( o, YBM_HANDLEVISCHILDREN, &hvc );

        return( TRUE );
        }

    return( FALSE );
}



_dispatcher( void, yg_YGM_ROTATEGUN, struct rotategun_msg  *rg )
{
/*
**  FUNCTION    Weil das mit dem Rotieren bei den Bordkanonen häufig auftritt
**              und viele daten privat sind, mach mr doch gleich ne Methode.
**              Wir bekommen eine Achse und einen Winkel, darum rotieren wir 
**              die Matrix und den Basisvektor.
**              Vielleicht sollten wir den Basisvektor in der Message wieder
**              zurückgeben, weil dann dir in yrd->gun aktualisiert werden
**              kann.
**
**  INPUT       rot - Achse
**              angle - Winkel, vorzeichenabhängig
**
**  RESULT      basis - aktualisierte Basis
**
**  CHANGED     af, das regnet und regnet, Petrus weiß, daß wir noch nicht fertig sind
*/

    struct ypagun_data *ygd;
    
    ygd = INST_DATA( cl, o );

    /*** Basis rotieren ***/
    yg_vec_round_vec( &(rg->rot), &(ygd->basis), rg->angle );

    /*** Basis in Message schreiben ***/
    rg->basis = ygd->basis;

    /*** Matrix rotieren ***/
    yg_dir_round_vec( &(rg->rot), &(ygd->bact->dir), rg->angle );
}
