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

