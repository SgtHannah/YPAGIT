/*
**  $Source: $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:24:47 $
**  $Locker: $
**  $Author: $
**
**
**
**  (C) Copyright 1995 by Andreas Flemming
*/
#include <exec/types.h>

#include <math.h>
#include <stdlib.h>

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "engine/engine.h"
#include "transform/tform.h"
#include "ypa/ypaworldclass.h"
#include "ypa/ypabactclass.h"
#include "input/input.h"


/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine


_dispatcher(BOOL, yb_YBM_SETPOSITION, struct setposition_msg *setpos)
{
    /*
    ** FUNCTION:    Setzt ein Objekt auf eine Position. Dabei sehen wir nach, ob
    **              es schon in einer BactList ist, nehmen es dort raus, klinken
    **              es in die neue Liste ein und setzen den Sektorpointer.
    **              Evtl. wird das Objekt, wenn es gewünscht ist, noch auf
    **              Bodenhöhe gesetzt.
    **
    ** RESULTS      TRUE, wenn die Position ok war, sonst FALSE
    **
    ** CHANGES      24.9.95 kriäted af
    **
    */

    struct ypabact_data *ybd;
    struct getsectorinfo_msg gsi;

    ybd = INST_DATA( cl, o );

    /* 'n Sektor holen */
    gsi.abspos_x = setpos->x;
    gsi.abspos_z = setpos->z;

    if( !_methoda( ybd->world, YWM_GETSECTORINFO, &gsi ) )  return( FALSE );

    /* abmelden */
    if( ybd->bact.Sector )
        _Remove( (struct Node *) &(ybd->bact.SectorNode) );

    /* neu anmelden */
    _AddTail( (struct List *) &(gsi.sector->BactList),
              (struct Node *) &(ybd->bact.SectorNode));

    ybd->bact.Sector = gsi.sector;
    ybd->bact.pos.x  = ybd->bact.old_pos.x = setpos->x;
    ybd->bact.pos.y  = ybd->bact.old_pos.y = setpos->y;
    ybd->bact.pos.z  = ybd->bact.old_pos.z = setpos->z;
    ybd->bact.SectX  = gsi.sec_x;
    ybd->bact.SectY  = gsi.sec_y;

    /*** Nachträgliche Korrektur ***/
    if( !(setpos->flags & YBFSP_DontCorrect ))
        _methoda( o, YBM_CORRECTPOSITION, NULL );

    return( TRUE );

}


_dispatcher( void, yb_YBM_CHECKPOSITION, void *nix )
{
/*
**  FUNCTION    Testet, ob die position Viewerfreundlich ist
**              und korrigiert sie entsprechend.
**              Aufruf nach set( Viewer, TRUE ), und evtl.
**              nach CRASHBACTERIUM.
**
**  INPUT/OUTPUT Nüscht
**
**  CHANGES     heute angefangen
*/

    struct ypabact_data *ybd;
    struct insphere_msg ins;
    struct Insect insect[ 10 ];
    FLOAT  radius, A, B, C, mdist, betrag;

    ybd = INST_DATA( cl, o );

    /*** 1. Ein Kugeltest ***/
    radius          = max( 32, ybd->bact.viewer_radius);
    ins.pnt.x       = ybd->bact.pos.x;
    ins.pnt.y       = ybd->bact.pos.y;
    ins.pnt.z       = ybd->bact.pos.z;
    ins.dof.x       = ybd->bact.dir.m11;
    ins.dof.y       = ybd->bact.dir.m12;
    ins.dof.z       = ybd->bact.dir.m13;
    ins.radius      = radius;
    ins.chain       = insect;
    ins.max_insects = 10;
    ins.flags       = 0;

    _methoda( ybd->world, YWM_INSPHERE, &ins );

    /* ------------------------------------------------------------------
    ** Nun das resultat auswerten. Ich ermittle aus allen Flächen, die
    ** ich geschnitten habe und die als Standfläche nicht in frage kommen
    ** (zu steil) einen Wegsetz-Vektor.
    ** ----------------------------------------------------------------*/
    A = B = C = mdist = betrag = 0.0;

    while( ins.num_insects-- ) {

        FLOAT dist;

        if( ins.chain[ ins.num_insects ].pln.B < YPS_PLANE ) {

            A += ins.chain[ ins.num_insects ].pln.A;
            B += ins.chain[ ins.num_insects ].pln.B;
            C += ins.chain[ ins.num_insects ].pln.C;

            dist = radius - nc_sqrt(
                   (ybd->bact.pos.x - ins.chain[ ins.num_insects ].ipnt.x) *
                   (ybd->bact.pos.x - ins.chain[ ins.num_insects ].ipnt.x) +
                   (ybd->bact.pos.y - ins.chain[ ins.num_insects ].ipnt.y) *
                   (ybd->bact.pos.y - ins.chain[ ins.num_insects ].ipnt.y) +
                   (ybd->bact.pos.z - ins.chain[ ins.num_insects ].ipnt.z) *
                   (ybd->bact.pos.z - ins.chain[ ins.num_insects ].ipnt.z) );

            if( (mdist == 0.0) || (mdist < dist) ) mdist = dist;
            }
        }

    /*** 2. test wegen Richtungsabhängigkeit des Kugeltests ***/
    radius          = max( 32, ybd->bact.viewer_radius);
    ins.pnt.x       = ybd->bact.pos.x;
    ins.pnt.y       = ybd->bact.pos.y;
    ins.pnt.z       = ybd->bact.pos.z;
    ins.dof.x       = -ybd->bact.dir.m11;
    ins.dof.y       = -ybd->bact.dir.m12;
    ins.dof.z       = -ybd->bact.dir.m13;
    ins.radius      = radius;
    ins.chain       = insect;
    ins.max_insects = 10;
    ins.flags       = 0;

    _methoda( ybd->world, YWM_INSPHERE, &ins );

    while( ins.num_insects-- ) {

        FLOAT dist;

        if( ins.chain[ ins.num_insects ].pln.B < YPS_PLANE ) {

            A += ins.chain[ ins.num_insects ].pln.A;
            B += ins.chain[ ins.num_insects ].pln.B;
            C += ins.chain[ ins.num_insects ].pln.C;

            dist = radius - nc_sqrt(
                   (ybd->bact.pos.x - ins.chain[ ins.num_insects ].ipnt.x) *
                   (ybd->bact.pos.x - ins.chain[ ins.num_insects ].ipnt.x) +
                   (ybd->bact.pos.y - ins.chain[ ins.num_insects ].ipnt.y) *
                   (ybd->bact.pos.y - ins.chain[ ins.num_insects ].ipnt.y) +
                   (ybd->bact.pos.z - ins.chain[ ins.num_insects ].ipnt.z) *
                   (ybd->bact.pos.z - ins.chain[ ins.num_insects ].ipnt.z) );

            if( (mdist == 0.0) || (mdist < dist) ) mdist = dist;
            }
        }

    /*** Jetzt haben wir die Resultate von 2  Schnitten --> Auswerten ***/

    betrag = nc_sqrt( A * A + B * B + C * C );

    if( betrag > 0.0001 ) {

        betrag = 1 / betrag;
        A *= betrag;
        B *= betrag;
        C *= betrag;
        }

    ybd->bact.pos.x -= A * mdist;
    ybd->bact.pos.y -= B * mdist;
    ybd->bact.pos.z -= C * mdist;
}


_dispatcher( void, yb_YBM_CORRECTPOSITION, void *nix )
{
    FLOAT border;
    BOOL  test = FALSE;
    struct ypabact_data *ybd;

    ybd = INST_DATA( cl, o );
    border = SECTOR_SIZE + 10;

    if( ybd->bact.pos.x > (ybd->bact.WorldX - border) ) {
        ybd->bact.pos.x = (ybd->bact.WorldX - border); test = TRUE; }
    if( ybd->bact.pos.x < border ) {
        ybd->bact.pos.x = border; test = TRUE; }
    if( ybd->bact.pos.z > -border ) {
        ybd->bact.pos.z = -border; test = TRUE; }
    if( ybd->bact.pos.z < (ybd->bact.WorldZ + border) ) {
        ybd->bact.pos.z = (ybd->bact.WorldZ + border); test = TRUE; }

    if( (ybd->flags & YBF_Viewer) && test &&
        (BCLID_YPATANK != ybd->bact.BactClassID) &&
        (BCLID_YPACAR  != ybd->bact.BactClassID) ) {

        /*** Eine Intersection ***/
        struct intersect_msg inter;
        inter.pnt.x = ybd->bact.pos.x;
        inter.pnt.y = ybd->bact.pos.y - 100;
        inter.pnt.z = ybd->bact.pos.z;
        inter.vec.x = 0.0;    // ybd->bact.pos.x - ybd->bact.old_pos.x;
        inter.vec.y = 100 + ybd->bact.viewer_over_eof; // ybd->bact.pos.y - ybd->bact.old_pos.y;
        inter.vec.z = 0.0;    // ybd->bact.pos.z - ybd->bact.old_pos.z;
        inter.flags = 0;
        _methoda( ybd->world, YWM_INTERSECT, &inter );
        if( inter.insect ) {

            /*** Vielleicht hilft hochsetzen ? ***/
            ybd->bact.pos.y = inter.ipnt.y - ybd->bact.viewer_over_eof;
            }
        }
}
