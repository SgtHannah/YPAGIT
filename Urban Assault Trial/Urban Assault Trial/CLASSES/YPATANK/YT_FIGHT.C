/*
**  $Source: $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:24:47 $
**  $Locker: $
**  $Author: $
**
**  Notwendig, wegen anderem Schussbereich der Panzer
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
#include "input/input.h"
#include "ypa/ypatankclass.h"


/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine


_dispatcher( BOOL, yt_YBM_CHECKAUTOFIREPOS, struct checkautofirepos_msg *msg)
{
/*
**  FUNCTION    Testet, ob wir in Abschußposition sind. Dabei müssen wir
**              unterscheiden, ob wir ein Bacterium oder einen Sektor
**              bekämpfen und welche Art von Waffe wir haben.
**              Für den Panzer gilt folgende Überlagerung: Wir testen
**              eine gasse, die auch etwas nach unten zeigt.
**
**  INPUT       Position und Art des Zieles
**
**  RESULT      TRUE, wenn günstig
**
**  CHANGED     kurz vor Jahresende     created     8100 000C
*/

    struct ypatank_data *ytd;
    struct flt_triple richtung;
    FLOAT  distance, sp_xz;
    struct WeaponProto *warray, *waffe;
    UBYTE  flags;
    struct intersect_msg inter;

    ytd = INST_DATA( cl, o);

    richtung.x = msg->target_pos.x - ytd->bact->pos.x;
    richtung.y = msg->target_pos.y - ytd->bact->pos.y;
    richtung.z = msg->target_pos.z - ytd->bact->pos.z;

    distance = nc_sqrt( richtung.x * richtung.x + richtung.y * richtung.y +
                        richtung.z * richtung.z );
    if( distance == 0 ) return( FALSE );

    richtung.x /= distance;
    richtung.y /= distance;
    richtung.z /= distance;

    /*** Waffeninformationen holen ***/
    _get( ytd->world, YWA_WeaponArray, &warray );
    
    /*** Testen, ob wir freie Sicht haben ***/
    ytd->free_lof = TRUE;
    inter.pnt.x = ytd->bact->pos.x;
    inter.pnt.y = ytd->bact->pos.y;
    inter.pnt.z = ytd->bact->pos.z;
    inter.vec.x = msg->target_pos.x - inter.pnt.x;
    inter.vec.y = msg->target_pos.y - inter.pnt.y;
    inter.vec.z = msg->target_pos.z - inter.pnt.z;
    inter.flags = 0;
   
    _methoda( ytd->world, YWM_INTERSECT2, &inter );
    if( inter.insect ) {
    
        /* -----------------------------------------------
        ** Um freie Sicht zu haben, muss das Ziel bei
        ** Kompaktsektoren der Sector (incl. Slurp) selbst
        ** sein, andernfalls der Subsektor. Bei bactzielen
        ** sollte vollkommen freie Sicht sein. Das sollte
        ** man aber vielleicht nicht beachten???
        ** ---------------------------------------------*/
        if( TARTYPE_BACTERIUM == msg->target_type ) {
        
            struct Bacterium *b = NULL;
            if( ytd->bact->SecTargetType == TARTYPE_BACTERIUM )
                b = ytd->bact->SecondaryTarget.Bact;
            else
                b = ytd->bact->PrimaryTarget.Bact;
        
            /*** nur wenn das Ziel Flak oder Tank ist ***/
            if( b && 
               ((BCLID_YPATANK == b->BactClassID) ||
                (BCLID_YPACAR  == b->BactClassID) ||
                (BCLID_YPAGUN  == b->BactClassID)) )
                ytd->free_lof = FALSE;
            }
        else {
        
            struct getsectorinfo_msg gsi_i, gsi_t;
        
            gsi_t.abspos_x = msg->target_pos.x;
            gsi_t.abspos_z = msg->target_pos.z;
            gsi_i.abspos_x = inter.ipnt.x;
            gsi_i.abspos_z = inter.ipnt.z;
            if( _methoda( ytd->world, YWM_GETSECTORINFO, &gsi_t ) &&
                _methoda( ytd->world, YWM_GETSECTORINFO, &gsi_i) ) {
            
                /*** Wenn's nicht mal der Sector ist... ***/
                if( gsi_t.sector != gsi_i.sector )
                    ytd->free_lof = FALSE;
                else {
                
                    /* -----------------------------------------
                    ** Wenn es ein 3x3-Sector ist, dann solte
                    ** auch der richtige Subsector sein, bevor
                    ** wir schiessen koennen.
                    ** SLURP/2 abziehen und durch 4 teilen. muss
                    ** gleich sein
                    ** ---------------------------------------*/
                    if( SECTYPE_COMPACT != gsi_t.sector->SType ) {
                    
                        WORD ss_x_i, ss_x_t, ss_z_i, ss_z_t, sss;
                        
                        sss = SECTOR_SIZE/4;
                    
                        ss_x_i = (WORD)( (gsi_i.abspos_x - SLURP_WIDTH/2)/sss );
                        ss_z_i = (WORD)( (gsi_i.abspos_z + SLURP_WIDTH/2)/sss );
                        ss_x_t = (WORD)( (gsi_t.abspos_x - SLURP_WIDTH/2)/sss );
                        ss_z_t = (WORD)( (gsi_t.abspos_z + SLURP_WIDTH/2)/sss );
                        if( (ss_x_i != ss_x_t) || (ss_z_i != ss_z_t) ) {
                            ytd->free_lof = FALSE;
                            }
                        }
                    }
                }
            }
        }

    if( ytd->bact->auto_ID == NO_AUTOWEAPON ) {

        waffe = NULL;
        }
    else {

        waffe = &( warray[ ytd->bact->auto_ID ] );

        /*** autonome Waffe ??? ***/
        if( !(waffe->Flags & WPF_Autonom) )
            waffe = NULL;
        else
            flags = waffe->Flags & ~WPF_Autonom;
        }

    /*** Wenn waffe == NULL, dann kann nur noch das MG helfen ***/
    if( waffe == NULL ) {

        if( ytd->bact->mg_shot == NO_MACHINEGUN )
            return( FALSE );      // weil es keinen Sinn hat...
        else
            flags = WPF_Driven; // weil es sich so in etwa verhält.
        }

    /*** Nun testen ***/
    if( (richtung.x != 0.0) || (richtung.z != 0.0) )
        sp_xz = (richtung.x * ytd->bact->dir.m31 + richtung.z * ytd->bact->dir.m33) /
                nc_sqrt( richtung.x * richtung.x + richtung.z * richtung.z ) /
                nc_sqrt( ytd->bact->dir.m31 * ytd->bact->dir.m31 +
                         ytd->bact->dir.m33 * ytd->bact->dir.m33 );
    else
        sp_xz = 0.0;

    if( msg->target_type == TARTYPE_BACTERIUM ) {

        switch( flags ) {

            case 0:

                /*** Bombe (kann auch beim Tank wegen Mine sein!) ***/
                if( richtung.y > 0.98 )
                    return( TRUE );
                else
                    return( FALSE );
                break;      // aus Gewohnheit...

            case WPF_Impulse:

                /*** Granate, nur x-z-Richtung ***/
                if( (distance < SECTOR_SIZE) &&
                    ( sp_xz > 0.93 ) &&
                    ( richtung.y > -0.85 ) &&
                    ( richtung.y <  0.2 ) )
                    return( TRUE );
                else
                    return( FALSE );
                break;

            default:

                /*** Nur die Richtung und die Entfernung testen ***/
                if( (distance < SECTOR_SIZE) &&
                    ( sp_xz > 0.93 ) &&
                    ( richtung.y > -0.85 ) &&
                    ( richtung.y <  0.2 ) )
                    return( TRUE );
                else
                    return( FALSE );
                break;
            }
        }
    else {

        if( waffe == NULL ) return( FALSE );    // weil wir nur mit MGs keine
                                                // Sektoren bekämpfen können
        switch( flags ) {

            case 0:

                /*** Bombe ***/
                if( richtung.y > 0.92 )
                    return( TRUE );
                else
                    return( FALSE );
                break;

            case WPF_Impulse:

                /*** Granate, nur x-z-Richtung ***/
                if( (distance < SECTOR_SIZE) &&
                    ( sp_xz > 0.91 ) &&
                    ( richtung.y > -0.4 ) &&
                    ( richtung.y <  0.3 ) )
                    return( TRUE );
                else
                    return( FALSE );
                break;

            default:

                /*** Nur die Richtung und die Entfernung testen ***/
                if( (distance < SECTOR_SIZE) &&
                    ( sp_xz > 0.91 ) &&
                    ( richtung.y > -0.4 ) &&
                    ( richtung.y <  0.3 ) )
                    return( TRUE );
                else
                    return( FALSE );
                break;
            }
        }
}

