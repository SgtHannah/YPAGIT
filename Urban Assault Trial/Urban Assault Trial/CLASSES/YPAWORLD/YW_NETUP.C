/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_listview.c,v $
**  $Revision: 38.8 $
**  $Date: 1996/03/21 20:30:55 $
**  $Locker:  $
**  $Author: floh $
**
**  Routinen für das Updaten von Informationen ueber Vehicle
**  ueber das Netz und zugehoerige Supportroutinen
**
**  (C) Copyright 1995/6/7/8 by A.Flemming
*/
#include <exec/types.h>
#include <exec/memory.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "engine/engine.h"
#include "ypa/ypaworldclass.h"
#include "ypa/guilist.h"
#include "ypa/ypagui.h"
#include "visualstuff/ov_engine.h"
#include "yw_protos.h"
#include "yw_gsprotos.h"
#include "ypa/yparoboclass.h"
#include "ypa/ypamissileclass.h"
#include "ypa/ypagunclass.h"
#include "bitmap/bitmapclass.h"

#ifdef __NETWORK__

#include "network/networkclass.h"
#include "ypa/ypamessages.h"
#include "thirdparty/eulerangles.h"
#include "yw_netprotos.h"


FLOAT max_dv = 0.0;

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_input_engine
_extern_use_ov_engine
_extern_use_audio_engine


/* -----------------------------------------------------------------------
** DebugDaten. Ich lege 2 Arrays an, wo alle erzeugten und alle zerstörten
** Vehicle und raketen reinkommen. Wenn eines nicht gefunden wurde, so
** kann ich daran entscheiden, ob das vehicle jemals erzeugt bzw. schon
** wieder freigegeben wurde
** ---------------------------------------------------------------------*/
WORD  NUM_DV  = 4000;
WORD  COFFSET = 0;
WORD  DOFFSET = 0;
ULONG CREATED[ 7 ][ 4000 ];
ULONG DESTROYED[ 7 ][ 4000 ];

BOOL yw_WasCreated( UBYTE owner, ULONG ident )
{
    int i;
    for( i = 0; i < COFFSET; i++ ) {
        if( CREATED[ owner ][ i ] == ident ) return( TRUE );
        }
    return( FALSE );
}

void yw_AddCreated( UBYTE owner, ULONG ident )
{
    if( COFFSET >= NUM_DV ) COFFSET = 0;
    CREATED[ owner ][ COFFSET ] = ident;
    COFFSET++;
}

BOOL yw_WasDestroyed( UBYTE owner, ULONG ident )
{
    int i;
    for( i = 0; i < DOFFSET; i++ ) {
        if( DESTROYED[ owner ][ i ] == ident ) return( TRUE );
        }
    return( FALSE );
}

void yw_AddDestroyed( UBYTE owner, ULONG ident )
{
    if( DOFFSET >= NUM_DV ) DOFFSET = 0;
    DESTROYED[ owner ][ DOFFSET ] = ident;
    DOFFSET++;
}

BOOL yw_UserRobo( struct Bacterium *robo )
{
    /* ----------------------------------------------------------------------
    ** Testet, ob der Robo UserRobo ist, denn für diesen werden keine Vehicle
    ** gemeldet und können somit auch nicht gefunden werden
    ** --------------------------------------------------------------------*/
    struct ypaworld_data *ywd;
    Object *world;

    _get( robo->BactObject, YBA_World, &world );

    ywd = INST_DATA( ((struct nucleusdata *)world)->o_Class, world );
    if( ywd->UserRobo == robo->BactObject )
        return( TRUE );
    else
        return( FALSE );
}

ULONG yw_GetVehicleNumber( struct Bacterium *robo )
{
    /* --------------------------------------------------------
    ** Anzahl der vehicle
    ** ------------------------------------------------------*/

    struct OBNode *com, *slave;
    ULONG  count = 1;   // robo

    com = (struct OBNode *) robo->slave_list.mlh_Head;
    while( com->nd.mln_Succ ) {

        slave = (struct OBNode *) com->bact->slave_list.mlh_Head;
        count++;

        while( slave->nd.mln_Succ ) {

            count++;
            slave = (struct OBNode *) slave->nd.mln_Succ;
            }

        com = (struct OBNode *) com->nd.mln_Succ;
        }

    return( count );
}


void yw_TalkAboutError( struct Bacterium *robo, ULONG ident )
{
    if( !yw_UserRobo( robo ) ) {

        if( yw_WasCreated( robo->Owner, ident ) )
            yw_NetLog("\n--- shadow vehicle %d was created\n", ident );
        else
            yw_NetLog("\n--- shadow vehicle %d was NOT created\n", ident );

        if( yw_WasDestroyed( robo->Owner, ident ) )
            yw_NetLog("\n--- shadow vehicle %d was just destroyed\n", ident );
        else
            yw_NetLog("\n--- shadow vehicle %d was NOT destroyed\n", ident );
        }
}

struct Bacterium *yw_GetBactByID( struct Bacterium *robo, ULONG ident )
{
    /* --------------------------------------------------------
    ** sucht aus der Liste des übergebenen Vehicles das vehicle
    ** mit der entsprechenden ID raus, wobei es auch die Root
    ** selbst sein kann.
    ** ------------------------------------------------------*/

    struct OBNode *com, *slave;

    if( robo->ident == ident ) return( robo );

    com = (struct OBNode *) robo->slave_list.mlh_Head;
    while( com->nd.mln_Succ ) {

        if( com->bact->ident == ident ) return( com->bact );

        slave = (struct OBNode *) com->bact->slave_list.mlh_Head;

        while( slave->nd.mln_Succ ) {

            if( slave->bact->ident == ident ) return( slave->bact );
            slave = (struct OBNode *) slave->nd.mln_Succ;
            }

        com = (struct OBNode *) com->nd.mln_Succ;
        }

    /*** Nichts gefunden ***/
    yw_TalkAboutError( robo, ident );
    return( NULL );
}



struct Bacterium *yw_GetBactByID_3( struct Bacterium *robo, ULONG ident )
{
    /* -------------------------------------------------------
    ** spezielle Version, die 3 Ebenen sucht. Es kann nämlich
    ** passieren, daß ein Commander mit Slaves auf Slaveniveau
    ** gesetzt wird.
    ** -----------------------------------------------------*/

    struct OBNode *com, *slave, *unterslave;

    if( robo->ident == ident ) return( robo );

    com = (struct OBNode *) robo->slave_list.mlh_Head;
    while( com->nd.mln_Succ ) {

        if( com->bact->ident == ident ) return( com->bact );

        slave = (struct OBNode *) com->bact->slave_list.mlh_Head;

        while( slave->nd.mln_Succ ) {

            if( slave->bact->ident == ident ) return( slave->bact );

            unterslave = (struct OBNode *) slave->bact->slave_list.mlh_Head;
            while( unterslave->nd.mln_Succ ) {

                if( unterslave->bact->ident == ident ) return( unterslave->bact );
                unterslave = (struct OBNode *) unterslave->nd.mln_Succ;
                }

            slave = (struct OBNode *) slave->nd.mln_Succ;
            }

        com = (struct OBNode *) com->nd.mln_Succ;
        }

    /*** Nichts gefunden ***/
    yw_TalkAboutError( robo, ident );
    return( NULL );
}


struct Bacterium *yw_GetBactByIDAndInfo( struct Bacterium *robo, ULONG ident,
                                         UBYTE kind, ULONG commander )
{
    /* --------------------------------------------------------
    ** sucht aus der Liste des übergebenen Vehicles das vehicle
    ** mit der entsprechenden ID raus, wobei es auch die Root
    ** selbst sein kann. Zur Unterstützung der Suche gibt es
    ** eine Info, wo sich das vehicle befindet.
    ** ------------------------------------------------------*/

    struct OBNode *com, *slave;

    switch( kind ) {

        case SI_ROBO:

            if( robo->ident == ident )
                return( robo );
            break;

        case SI_COMMANDER:

            com = (struct OBNode *) robo->slave_list.mlh_Head;
            while( com->nd.mln_Succ ) {

                if( com->bact->ident == ident ) return( com->bact );

                com = (struct OBNode *) com->nd.mln_Succ;
                }
            break;

        case SI_SLAVE:

            /*** zuerst den commander suchen... ***/
            com = (struct OBNode *) robo->slave_list.mlh_Head;
            while( com->nd.mln_Succ ) {

                if( com->bact->ident == commander ) {

                    /*** Nur dessen Slaves kommen in Betracht ***/
                    slave = (struct OBNode *) com->bact->slave_list.mlh_Head;

                    while( slave->nd.mln_Succ ) {

                        if( slave->bact->ident == ident ) return( slave->bact );
                        slave = (struct OBNode *) slave->nd.mln_Succ;
                        }

                    /*** Nicht gefunden, obwohl es der Commander war ***/
                    break;
                    }

                com = (struct OBNode *) com->nd.mln_Succ;
                }

            break;

        case SI_UNKNOWN:

            /*** Dann eben klassisch ***/
            return( yw_GetBactByID( robo, ident ) );
            break;
        }

    /*** Nüscht gefunden ***/
    yw_TalkAboutError( robo, ident );
    return( NULL );
}


struct Bacterium *yw_GetMissileBactByRifleman( struct Bacterium *robo,
                                               ULONG ident, ULONG rifleman )
{
    /* ---------------------------------------------
    ** Sucht waffe anhand des Wirtes. geht schneller
    ** -------------------------------------------*/

    struct OBNode *com, *slave, *missile;

    if( robo->ident == rifleman ) {

        missile = (struct OBNode *) robo->auto_list.mlh_Head;
        while( missile->nd.mln_Succ ) {

            if( missile->bact->ident == ident ) return( missile->bact );
            missile = (struct OBNode *) missile->nd.mln_Succ;
            }

        yw_TalkAboutError( robo, ident );
        return( NULL );
        }

    com = (struct OBNode *) robo->slave_list.mlh_Head;
    while( com->nd.mln_Succ ) {

        if( com->bact->ident == rifleman ) {

            missile = (struct OBNode *) com->bact->auto_list.mlh_Head;
            while( missile->nd.mln_Succ ) {

                if( missile->bact->ident == ident ) return( missile->bact );
                missile = (struct OBNode *) missile->nd.mln_Succ;
                }

            yw_TalkAboutError( robo, ident );
            return( NULL );
            }

        slave = (struct OBNode *) com->bact->slave_list.mlh_Head;

        while( slave->nd.mln_Succ ) {

            if( slave->bact->ident == rifleman ) {

                missile = (struct OBNode *) slave->bact->auto_list.mlh_Head;
                while( missile->nd.mln_Succ ) {

                    if( missile->bact->ident == ident ) return( missile->bact );
                    missile = (struct OBNode *) missile->nd.mln_Succ;
                    }

                yw_TalkAboutError( robo, ident );
                return( NULL );
                }

            slave = (struct OBNode *) slave->nd.mln_Succ;
            }

        com = (struct OBNode *) com->nd.mln_Succ;
        }

    /*** Nichts gefunden ***/
    yw_TalkAboutError( robo, ident );
    return( NULL );
}


struct Bacterium *yw_GetMissileBactByID( struct Bacterium *robo, ULONG ident )
{
    /* -------------------------------------------------------
    ** Fast wie die vorherige Routine, nur suchen wir Raketen,
    ** die in den Sublisten der Vehicle hängen.
    ** -----------------------------------------------------*/

    struct OBNode *com, *slave, *missile;

    missile = (struct OBNode *) robo->auto_list.mlh_Head;
    while( missile->nd.mln_Succ ) {

        if( missile->bact->ident == ident ) return( missile->bact );
        missile = (struct OBNode *) missile->nd.mln_Succ;
        }

    com = (struct OBNode *) robo->slave_list.mlh_Head;
    while( com->nd.mln_Succ ) {

        missile = (struct OBNode *) com->bact->auto_list.mlh_Head;
        while( missile->nd.mln_Succ ) {

            if( missile->bact->ident == ident ) return( missile->bact );
            missile = (struct OBNode *) missile->nd.mln_Succ;
            }

        slave = (struct OBNode *) com->bact->slave_list.mlh_Head;

        while( slave->nd.mln_Succ ) {

            missile = (struct OBNode *) slave->bact->auto_list.mlh_Head;
            while( missile->nd.mln_Succ ) {

                if( missile->bact->ident == ident ) return( missile->bact );
                missile = (struct OBNode *) missile->nd.mln_Succ;
                }

            slave = (struct OBNode *) slave->nd.mln_Succ;
            }

        com = (struct OBNode *) com->nd.mln_Succ;
        }

    /*** Nichts gefunden ***/
    yw_TalkAboutError( robo, ident );
    return( NULL );
}


struct OBNode *yw_GetRoboByOwner( struct ypaworld_data *ywd, UBYTE owner )
{
    /*** Sucht den Robo aus der CmdList, beachtet BactClassID ***/
    struct OBNode *robo;

    robo = (struct OBNode *) ywd->CmdList.mlh_Head;
    while( robo->nd.mln_Succ ) {

        if( (robo->bact->Owner == (UBYTE) owner) &&
            (BCLID_YPAROBO     == robo->bact->BactClassID) ) {

            return( robo );
            }

        robo = (struct OBNode *) robo->nd.mln_Succ;
        }

    return( NULL );
}


void yw_RemoveAttacker( struct Bacterium *bact )
{
    struct OBNode *attacker;
    struct List *attck_list;

    _get( bact->BactObject, YBA_AttckList, &attck_list );

    while( attacker = (struct OBNode *) _RemHead( attck_list ) ) {

        /* -----------------------------------------------------------------
        ** Um SETTARGET verwenden zu können, muß vorher TARTYPE_NONE gesetzt
        ** werden, weil wir aus der Liste schon raus sind!
        **
        ** Es scheint so, als wäre der dritte und größte und bösartigste
        ** Remove-Bug gefunden. Siehe dazu die if-Statements. 5-6 Stunden
        ** habe ich im letzten Anlauf benötigt, und selbst als ich ihn
        ** gefunden hatte, hat er sich noch über 1/2 Stunde gewehrt. Aber
        ** jetzt ist er exorziert und Amok geht einer lichten Zukunft ent-
        ** gegen!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        **      af 11.11. 96   0.05 Uhr
        ** ---------------------------------------------------------------*/
        struct OBNode *snode, *pnode;
        _get( attacker->o, YBA_PtAttckNode, &pnode );
        _get( attacker->o, YBA_StAttckNode, &snode );

        //if( attacker->bact->PrimaryTarget.Bact == &(ybd->bact) ) {
        if( pnode == attacker ) {

            /*** Wir waren sein Hauptziel ***/
            attacker->bact->PrimaryTarget.Bact = NULL;
            attacker->bact->PrimTargetType = TARTYPE_NONE;
            attacker->bact->assess_time    = 0;
            }
        else {

            /*** else: kann ja nur eine der beiden Nodes Node gewesen sein ***/

            //if( attacker->bact->SecondaryTarget.Bact == &(ybd->bact) ) {
            if( snode == attacker ) {

                /* Wir waren (auch) sein Nebenziel. Da ist alles  egal */
                attacker->bact->SecondaryTarget.Bact = NULL;
                attacker->bact->SecTargetType = TARTYPE_NONE;
                attacker->bact->assess_time   = 0;
                }
            else {
                yw_NetLog("Hein Blöd\n");
                }
            }
        }
}


void yw_ReleaseWeapon( struct ypaworld_data *ywd, struct Bacterium *bact )
{
    /* -------------------------------------------------------------
    ** Gibt die Waffen des Bacteriums frei. Dazu gehört auch eine
    ** Zielabmeldung. Diese Routine wird auch vom ROBODIE-Block
    ** aufgerufen und von der entgültigen Freigabe, die ja auch nach
    ** fehlern erfolgt.
    ** -----------------------------------------------------------*/
    while( bact->auto_list.mlh_Head->mln_Succ ) {

        struct Node   *node;
        struct OBNode *waffe = (struct OBNode *) bact->auto_list.mlh_Head;
        
        /*** Raketen sind original, koennen also BactZiele haben ***/
        if( TARTYPE_BACTERIUM == waffe->bact->PrimTargetType ) {
        
            struct Node *attck_node;
            _get( waffe->o, YBA_PtAttckNode, &attck_node );
            
            _Remove( (struct Node *) attck_node );
            waffe->bact->PrimTargetType = TARTYPE_NONE;
            }

        /*** Abmelden von Angreifern ***/
        yw_RemoveAttacker( waffe->bact );

        /*** Waffe selbst Ausklinken ***/
        _get( waffe->o, YMA_AutoNode, &node );
        _Remove( node );
        waffe->bact->master = NULL;

        /*** Freigeben ***/
        _methoda( ywd->world, YWM_RELEASEVEHICLE, waffe->o);
        
        /*** Auch Weapons "Logic-deathen" ! ***/
        waffe->bact->ExtraState |= EXTRA_LOGICDEATH;
        }
}


void yw_DisConnectFromRobo( struct ypaworld_data *ywd, struct OBNode *vehicle )
{
    /* ----------------------------------------------------------------
    ** Testet, ob es sich um eine Bordflak handelt, die sich natuerlich
    ** aus dem RoboArray verabschieden muss
    ** --------------------------------------------------------------*/
    if( BCLID_YPAGUN == vehicle->bact->BactClassID ) {
    
        ULONG rgun;
    
        _get( vehicle->o, YGA_RoboGun, &rgun );
        if( rgun ) {
        
            struct gun_data *g_array;
            int    i;
    
            _get( vehicle->bact->robo, YRA_GunArray, &g_array );
    
            for( i = 0; i < NUMBER_OF_GUNS; i++ )
                if( g_array[ i ].go == vehicle->o )  g_array[ i ].go = NULL;
            }
        }
}



void yw_InsertVehicleData( struct Bacterium *bact,
                           struct ypamessage_vehicledata_i *vdm,
                           ULONG count, BOOL interpolate )
{
    /* -------------------------------------------------------
    ** Schreibt die daten in den großen Block. dabei gehen wir
    ** von der Interplationsmessage aus, die fuer beide gleich
    ** ist. Nur wenn wir extrapolieren, schreiben wir noch die
    ** geschwindigkeit mit rein.
    ** -----------------------------------------------------*/
    EulerAngles ea;
    HMatrix hm;
    ULONG   VIEW;
    struct vehicledata_i *block;

    if( interpolate )
        block = &(vdm->block[ count ]);
    else
        block = (struct vehicledata_i *) &(((struct ypamessage_vehicledata_e *)vdm)->block[ count ]);

    if( count >= MAXNUM_TRANSFEREDVEHICLES ) {

        if( count == MAXNUM_TRANSFEREDVEHICLES )
            yw_NetLog("Vehicle Buffer overflow!!! more than 500 vehicles\n");
        return;
        }

    /*** Pos und Speed ***/
    vdm->block[ count ].i.pos_x = (WORD)(((LONG)(bact->pos.x)) >> 1);
    vdm->block[ count ].i.pos_y = (WORD)(((LONG)(bact->pos.y)) >> 1);
    vdm->block[ count ].i.pos_z = (WORD)(((LONG)(bact->pos.z)) >> 1);

    if( FALSE == interpolate ) {
        struct ypamessage_vehicledata_e *v2;
        v2 = (struct ypamessage_vehicledata_e *) vdm;

        v2->block[ count ].speed.x = bact->dof.v * bact->dof.x;
        v2->block[ count ].speed.y = bact->dof.v * bact->dof.y;
        v2->block[ count ].speed.z = bact->dof.v * bact->dof.z;
        }

    /*** Viewer-Korrektur ***/
    _get( bact->BactObject, YBA_Viewer, &VIEW );
    if( VIEW && (bact->ExtraState & EXTRA_LANDED) ) {

        FLOAT len = bact->viewer_over_eof - bact->over_eof;
        /* ----------------------------------------------------------------
        ** Aus irgendeinem Grunde habe ich bei Panzern die korrekte,
        ** an lokal-y orientierte Boden-Korrektur wieder auskommentiert und
        ** an global-y orientiert. Da ich keine Ahnung habe, warum, mache
        ** ich es hier lieber auch so.
        **
        ** vdm->block[ count ].pos.x += bact->dir.m21 * len;
        ** vdm->block[ count ].pos.y += bact->dir.m22 * len;
        ** vdm->block[ count ].pos.z += bact->dir.m23 * len;
        ** --------------------------------------------------------------*/
        block->i.pos_y += (WORD)( ((LONG)len) >> 1);
        }

    /* ---------------------------------------------------------------------
    ** Flaks können, wenn der User drinnen sitzt, extrem flackern. Ich
    ** merke mir dann aber die Position in old_pos. Weiterhin ist bei
    ** Flaks die Position lediglich beim Fallen interessant. Dann hängt
    ** old_pos aber nur um einen Frame hinterher. Na erraten? Ich
    ** kann es mir leisten, bei Guns die old_pos zu verschicken. Genial, ne?
    ** -------------------------------------------------------------------*/
    if( BCLID_YPAGUN == bact->BactClassID ) {
        block->i.pos_x = (WORD)( ((LONG)(bact->old_pos.x)) >> 1);
        block->i.pos_y = (WORD)( ((LONG)(bact->old_pos.y)) >> 1);
        block->i.pos_z = (WORD)( ((LONG)(bact->old_pos.z)) >> 1);
                 
        block->i.specialinfo  |= (UBYTE) SIF_YPAGUN;
        }
    else
        block->i.specialinfo  &= (UBYTE)~SIF_YPAGUN;

    /*** Matrix packen in Eulerwinkel ***/
    hm[0][0]=bact->dir.m11; hm[0][1]=bact->dir.m12; hm[0][2]=bact->dir.m13; hm[0][3]=0.0;
    hm[1][0]=bact->dir.m21; hm[1][1]=bact->dir.m22; hm[1][2]=bact->dir.m23; hm[1][3]=0.0;
    hm[2][0]=bact->dir.m31; hm[2][1]=bact->dir.m32; hm[2][2]=bact->dir.m33; hm[2][3]=0.0;
    hm[3][0]=0.0;           hm[3][1]=0.0;           hm[3][2]=0.0;           hm[3][3]=1.0;
    ea = Eul_FromHMatrix(hm,EulOrdXYZs);
    block->i.roll  = (UBYTE) FLOAT_TO_INT(((ea.x/(2*PI))*127.0));
    block->i.pitch = (UBYTE) FLOAT_TO_INT(((ea.y/(2*PI))*127.0));
    block->i.yaw   = (UBYTE) FLOAT_TO_INT(((ea.z/(2*PI))*127.0));

    /*** Bei Bedarf noch ident und Klasse ***/
    block->i.ident  = bact->ident;
    block->i.specialinfo  &= (UBYTE)~SIF_YPAMISSY;

    /*** weitere Specialinfos ***/
    if( bact->ExtraState & EXTRA_DONTRENDER )
        block->i.specialinfo |= (UBYTE) SIF_DONTRENDER;
    else
        block->i.specialinfo &= (UBYTE)~SIF_DONTRENDER;

    if( bact->ExtraState & EXTRA_LANDED )
        block->i.specialinfo |= (UBYTE) SIF_LANDED;
    else
        block->i.specialinfo &= (UBYTE)~SIF_LANDED;

    if( bact->BactClassID == BCLID_YPAROBO )
        block->i.specialinfo |= (UBYTE) SIF_YPAROBO;
    else
        block->i.specialinfo &= (UBYTE)~SIF_YPAROBO;

    if( bact->ExtraState & EXTRA_SETDIRECTLY )
        block->i.specialinfo |= (UBYTE) SIF_SETDIRECTLY;
    else
        block->i.specialinfo &= (UBYTE)~SIF_SETDIRECTLY;
    bact->ExtraState &= ~EXTRA_SETDIRECTLY;


    /*** Energie ist jetzt fuer das HUD notwendig ***/
    block->i.energy = bact->Energy;
    block->i.p      = (WORD)0;

}


BOOL yw_CollectVehicleData( struct ypaworld_data *ywd,
                            struct ypamessage_vehicledata_i *vdm )
{
    /* -----------------------------------------------------------------
    ** Sammelt die daten zu allen vehiclen. Dabei schreiben wir zu
    ** unserem Robo alle daten inklusive der Raketen nacheinander
    ** in die Struktur. Auf der anderen Seite wird alles wieder
    ** so ausgelesen. 
    ** Wenn es sich lohnt, Daten zu verschicken, dann geben wir TRUE
    ** zurück. Nicht lohnt es sich, wenn der Robo nicht mehr da ist oder
    ** er tot ist und keine Slaves mehr hat! (je nachdem, ob Viewer
    ** im Robo sitzt oder nicht)
    **
    ** Ich uebergebe einfach den Pointer fuer die Interpolation, der 
    ** kann bei Bedarf gecastet werden. Beide Strukturen sind am Anfang
    ** gleich.
    ** ---------------------------------------------------------------*/

    struct OBNode *robo, *com, *slave;
    ULONG  count = 0, i, max_items;
    BOOL   found = FALSE;
    
    max_items = MAXNUM_TRANSFEREDVEHICLES - 1;

    /*** Robo ermitteln ***/
    robo = (struct OBNode *) ywd->CmdList.mlh_Head;
    while( robo->nd.mln_Succ ) {

        if( robo->o == ywd->UserRobo ) {
            found = TRUE; break; }
        robo = (struct OBNode *) robo->nd.mln_Succ;
        }

    if( !found ) {

        yw_NetLog("CollectVehicleData: My Robo doesn't exist!\n");
        return( FALSE );
        }

    /*** Robo reinschreiben ***/
    yw_InsertVehicleData( robo->bact, vdm, count++, (BOOL)ywd->interpolate );
    vdm->generic.owner = robo->bact->Owner;

    /*** Alle Untergebenen mit raketen reinschreiben ***/
    com = (struct OBNode *) robo->bact->slave_list.mlh_Head;
    while( com->nd.mln_Succ ) {

        /*** Commander rein ***/
        if( count >= max_items ) break;
        yw_InsertVehicleData( com->bact, vdm, count++, (BOOL)ywd->interpolate );

        slave = (struct OBNode *) com->bact->slave_list.mlh_Head;
        while( slave->nd.mln_Succ ) {

            /*** SlaveDaten reinschreiben ***/
            if( count >= max_items ) break;
            yw_InsertVehicleData( slave->bact, vdm, count++, (BOOL)ywd->interpolate );

            /*** Next Slave ***/
            slave = (struct OBNode *) slave->nd.mln_Succ;
            }

        /*** Next Commander ***/
        com = (struct OBNode *) com->nd.mln_Succ;
        }

    vdm->head.number  = count;

    /*** Debug checks ***/
    for( i = 0; i < vdm->head.number; i++ ) {

        if( vdm->block[i].i.ident < 65535 )
            yw_NetLog("\n+++ CVD: i send nonsens! (ident %d)\n", vdm->block[i].i.ident);
        }
    if( !ywd->interpolate )
        yw_NetLog("\n+++ CVD: Extrapolating ????\n");

    return( TRUE );
}


void yw_ReadVehicleData_E( struct Bacterium *bact,
                         struct ypaworld_data *ywd,
                         struct ypamessage_vehicledata_e *vdm,
                         ULONG  count, LONG timestamp )
{
    /* --------------------------------------------------------------
    ** Liest die daten aus dem count-ten Datenelement der vdm-message
    ** in das zugehörige vehicle
    ** ------------------------------------------------------------*/
    EulerAngles ea;
    HMatrix hm;
    struct flt_m3x3 neu_dir;
    LONG   lnet_time;
    FLOAT  net_time, b;
    struct flt_triple new_speed;


    if( count >= vdm->head.number ) return;

    /*** ----------------------  Z E I T -------------------- ***/

    /* -----------------------------------------------------------
    ** Die Updates sind abhängig von Eigentümer und auch von ihrer
    ** Erzeugung. Deshalb merken wir uns die zeit seit dem letzten
    ** Auffrischen für jedes Vehicle einzeln. Ebenso wird diese
    ** beim Erzeugen gesetzt, um bei der Extrapolation des ersten
    ** Frames keine unangenehmen Effekte zu bekommen.
    ** Es kann sein, daß Erzeugungs- und Update-Messages zur glei-
    ** chen Zeit reinkommen. Dann ignorieren wir das eben...
    ** ---------------------------------------------------------*/
    lnet_time = timestamp - bact->last_frame;
    if( lnet_time <= 2 ) return;
    net_time = ((FLOAT)lnet_time) / 1000.0;

    /*** ------------------  P O S I T I O N ---------------- ***/

    /*** Position übernehmen ***/
    bact->old_pos = bact->pos;
    //bact->pos     = vdm->block[ count ].e.pos;
    bact->pos.x   = (FLOAT)(vdm->block[ count ].e.pos_x << 1);
    bact->pos.y   = (FLOAT)(vdm->block[ count ].e.pos_y << 1);
    bact->pos.z   = (FLOAT)(vdm->block[ count ].e.pos_z << 1);

    /*** ---------- G E S C H W I N D I G K E I T ------------- ***/

    /* ---------------------------------------------------------
    ** Aus der Wegänderung im letzten Frame ermittle ich, welche
    ** Geschwindigkeit ich da hatte.
    ** -------------------------------------------------------*/
    new_speed = vdm->block[ count ].speed;

    /* -------------------------------------------------------------
    ** Aus dieser geschwindigkeit und der vorherigen Geschwindigkeit
    ** ermittle ich die Änderung der geschwindigkeitsänderung, auf
    ** deutsch die Beschleunigung.
    ** -----------------------------------------------------------*/
    bact->d_speed.x = (new_speed.x - bact->dof.x * bact->dof.v) / net_time;
    bact->d_speed.y = (new_speed.y - bact->dof.y * bact->dof.v) / net_time;
    bact->d_speed.z = (new_speed.z - bact->dof.z * bact->dof.v) / net_time;

    /*** Korrektur ***/
    b = nc_sqrt( bact->d_speed.x * bact->d_speed.x + bact->d_speed.y *
                 bact->d_speed.y + bact->d_speed.z * bact->d_speed.z);
    if( b > max_dv &&
        (BCLID_YPAMISSY != bact->BactClassID) ) {

        yw_NetLog("d_speed max: %10.5f / class %d dof.v old %10.5f with net_time %4.3f\n",
                 b, bact->BactClassID, bact->dof.v, net_time );
        max_dv = b;
        }

    if( b > 400.0 ) {

        bact->d_speed.x *= 4.0 / b;
        bact->d_speed.y *= 4.0 / b;
        bact->d_speed.z *= 4.0 / b;
        }


    if( (BCLID_YPAGUN  == bact->BactClassID) ||
        (BCLID_YPAROBO == bact->BactClassID) ) {

        /* ---------------------------------------------------------
        ** geschwindigkeit aus Positionsänderung ermitteln. Weil dof
        ** trotz alledem 0 sein kann. Bei nichttoten Robos x-z-
        ** Geschwindigkeit nachträglich auf 0 setzen, denn Guns
        ** können ja fallen...
        ** -------------------------------------------------------*/
        bact->dof.x = ( bact->pos.x - bact->old_pos.x ) / (net_time * METER_SIZE);
        bact->dof.y = ( bact->pos.y - bact->old_pos.y ) / (net_time * METER_SIZE);
        bact->dof.z = ( bact->pos.z - bact->old_pos.z ) / (net_time * METER_SIZE);

        bact->dof.v = nc_sqrt( bact->dof.x * bact->dof.x + bact->dof.y *
                               bact->dof.y + bact->dof.z * bact->dof.z );

        if( bact->dof.v > 0.001 ) {

            bact->dof.x /= bact->dof.v;
            bact->dof.y /= bact->dof.v;
            bact->dof.z /= bact->dof.v;
            }

        if( BCLID_YPAROBO == bact->BactClassID ) {
            bact->dof.x = bact->dof.z = 0.0; }

        /* -----------------------------------------------------------------
        ** Weil beim Triggern d_speed mit reingerechnet wird, löschen wir es
        ** besser...
        ** ---------------------------------------------------------------*/
        bact->d_speed.x = bact->d_speed.y = bact->d_speed.z = 0.0;
        }
    else {

        /*** Geschwindigkeit noch übernehmen ***/
        bact->dof.v = nc_sqrt( new_speed.x * new_speed.x +
                               new_speed.y * new_speed.y +
                               new_speed.z * new_speed.z );

        if( bact->dof.v > 0.001 ) {

            bact->dof.x = new_speed.x / bact->dof.v;
            bact->dof.y = new_speed.y / bact->dof.v;
            bact->dof.z = new_speed.z / bact->dof.v;
            }
        }

    /* -------------------------------------------------------------
    ** Positionen koennen sich ruckartig aendern (z.B. Robobeamen) 
    ** -----------------------------------------------------------*/
    if( vdm->block[ count ].e.specialinfo & SIF_SETDIRECTLY ) {

        bact->old_pos = bact->pos;
        bact->dof.v   = 0.0;
        }


    /*** -------------- A U S R I C H T U N G -------------- ***/

    /*** Matrix entpacken aus Eulerwinkel ***/
    ea.x = (((float)(vdm->block[ count ].e.roll))/127.0)*2*PI;
    ea.y = (((float)(vdm->block[ count ].e.pitch))/127.0)*2*PI;
    ea.z = (((float)(vdm->block[ count ].e.yaw))/127.0)*2*PI;
    ea.w = EulOrdXYZs;
    Eul_ToHMatrix(ea,hm);
    neu_dir.m11=hm[0][0]; neu_dir.m12=hm[0][1]; neu_dir.m13=hm[0][2];
    neu_dir.m21=hm[1][0]; neu_dir.m22=hm[1][1]; neu_dir.m23=hm[1][2];
    neu_dir.m31=hm[2][0]; neu_dir.m32=hm[2][1]; neu_dir.m33=hm[2][2];

    /*** Differenz-Matrix ermitteln ***/
    bact->d_matrix.m11 = (neu_dir.m11 - bact->old_dir.m11) / net_time;
    bact->d_matrix.m12 = (neu_dir.m12 - bact->old_dir.m12) / net_time;
    bact->d_matrix.m13 = (neu_dir.m13 - bact->old_dir.m13) / net_time;
    bact->d_matrix.m21 = (neu_dir.m21 - bact->old_dir.m21) / net_time;
    bact->d_matrix.m22 = (neu_dir.m22 - bact->old_dir.m22) / net_time;
    bact->d_matrix.m23 = (neu_dir.m23 - bact->old_dir.m23) / net_time;
    bact->d_matrix.m31 = (neu_dir.m31 - bact->old_dir.m31) / net_time;
    bact->d_matrix.m32 = (neu_dir.m32 - bact->old_dir.m32) / net_time;
    bact->d_matrix.m33 = (neu_dir.m33 - bact->old_dir.m33) / net_time;
    

    /*** Matrix übernehmen ***/
    bact->dir     = neu_dir;
    bact->old_dir = neu_dir;

    /*** SpecialInfo ***/
    if( vdm->block[ count ].e.specialinfo & SIF_DONTRENDER )
        bact->ExtraState |= EXTRA_DONTRENDER;
    if( vdm->block[ count ].e.specialinfo & SIF_LANDED )
        bact->ExtraState |=  EXTRA_LANDED;
    else
        bact->ExtraState &= ~EXTRA_LANDED;

    /*** Zeit aktualisieren ***/
    bact->last_frame = timestamp;

    /* -------------------------------------------------------------------
    ** Gelandete und Panzer nachkorrigieren. Das machen wir, um die
    ** etwas andere Viewer-Boden-ausrichtung des Originals zu korrigieren.
    ** -----------------------------------------------------------------*/
    if( bact->ExtraState & EXTRA_LANDED ) {

        struct intersect_msg inter;
        inter.pnt.x = bact->pos.x;         inter.pnt.y = bact->pos.y;
        inter.pnt.z = bact->pos.z;         inter.vec.x = bact->dir.m21 * 200;
        inter.vec.y = bact->dir.m22 * 200; inter.vec.z = bact->dir.m23 * 200;
        inter.flags = 0;
        _methoda( ywd->world, YWM_INTERSECT, &inter );
        if( inter.insect ) {

            bact->pos.x = inter.ipnt.x - bact->dir.m21 * bact->over_eof;
            bact->pos.y = inter.ipnt.y - bact->dir.m22 * bact->over_eof;
            bact->pos.z = inter.ipnt.z - bact->dir.m23 * bact->over_eof;
            }
        }

    /*** ---------------------- E N E R G I E ----------------------- ***/
    bact->Energy = vdm->block[ count ].e.energy;
}


void yw_ReadVehicleData_I( struct Bacterium *bact,
                         struct ypaworld_data *ywd,
                         struct ypamessage_vehicledata_i *vdm,
                         ULONG  count, LONG timestamp )
{
    /* --------------------------------------------------------------
    ** Liest die daten aus dem count-ten Datenelement der vdm-message
    ** in das zugehörige vehicle
    ** ------------------------------------------------------------*/
    EulerAngles ea;
    HMatrix hm;
    struct flt_m3x3 neu_dir;
    FLOAT  net_time;
    struct flt_triple new_speed, p;
    BOOL setdirectly = FALSE;


    if( count >= vdm->head.number ) return;

    /*** ----------------------  Z E I T -------------------- ***/

    /* ---------------------------------------------------------
    ** Hier interpolieren wir von der position, wo wir sind, auf
    ** die, die uns übergeben wurde.
    ** -------------------------------------------------------*/
    net_time = ((FLOAT)vdm->head.diff_sendtime)/1000.0;

    /*** ---------- G E S C H W I N D I G K E I T ------------- ***/

    /* --------------------------------------------------------------------
    ** Ist der Vektor von old_pos auf pos, bezogen auf das
    ** Zeitintervall. Dazu nehme ich old_pos, wenn ich die Position
    ** in jedem Frame korrigiere, was eigentlich exakt ist. Da die Message-
    ** laufzeiten aber unterschiedlich sind, habe ich immer Fehler,
    ** die beim Korrigieren ruckeln. Deshalb ist eine andere Variante,
    ** dof von der jetzigen (evtl. falschen) Position auf die neue
    ** aus block hinzubiegen. Dann berechne ich aber nicht vom exakten
    ** old_pos aus, sondern vom tatsächlichen pos. Dann muß die Korrektur
    ** bact->pos = bact->old_pos deaktiviert sein und ich muß mir in
    ** old_pos das tatsächliche merken!
    ** ------------------------------------------------------------------*/
    p.x = (FLOAT)( vdm->block[ count ].i.pos_x << 1);
    p.y = (FLOAT)( vdm->block[ count ].i.pos_y << 1);
    p.z = (FLOAT)( vdm->block[ count ].i.pos_z << 1);

    if( (p.x < 0.0) || (p.x > bact->WorldX) ||
        (p.z > 0.0) || (p.z < bact->WorldZ))
        yw_NetLog("\n+++ EVD: impossible position x %7.2f(%d) z %7.2f(%d) of object %d\n",
                 p.x, p.z, vdm->block[ count ].i.pos_x, vdm->block[ count ].i.pos_z,
                 bact->ident );

    new_speed.x=(p.x-bact->old_pos.x)/(net_time*METER_SIZE);
    new_speed.y=(p.y-bact->old_pos.y)/(net_time*METER_SIZE);
    new_speed.z=(p.z-bact->old_pos.z)/(net_time*METER_SIZE);

    bact->dof.v = nc_sqrt( new_speed.x * new_speed.x +
                           new_speed.y * new_speed.y +
                           new_speed.z * new_speed.z );

    /*** Wenn Geschwindigkeitsbegrenzung, dann hier ***/

    if( bact->dof.v > 0.0001 ) {

        bact->dof.x = new_speed.x / bact->dof.v;
        bact->dof.y = new_speed.y / bact->dof.v;
        bact->dof.z = new_speed.z / bact->dof.v;
        }

    /* -------------------------------------------------------------------
    ** Speed nicht veraendern, sondern accel berechnen, die ich braeuchte,
    ** um vom derzeitigen speed auf das zukuenftige pos (in p) zu kommen.
    ** Die Formel ergibt sich aus s=a*2/(t*t)+dof*t+so
    ** -----------------------------------------------------------------*/
    //old_accel = bact->accel;
    //bact->accel.x = (p.x-bact->pos.x)/METER_SIZE - (bact->dof.x*bact->dof.v*net_time);
    //bact->accel.y = (p.y-bact->pos.y)/METER_SIZE - (bact->dof.y*bact->dof.v*net_time);
    //bact->accel.z = (p.z-bact->pos.z)/METER_SIZE - (bact->dof.z*bact->dof.v*net_time);
    //bact->accel.x *= (2.0/(net_time*net_time));
    //bact->accel.y *= (2.0/(net_time*net_time));
    //bact->accel.z *= (2.0/(net_time*net_time));
    //
    ///* ---------------------------------------------------------------
    //** Es kommen, wahrscheinlich durch die unt. zeiten, seltsame
    //** beschleunigungen zustande. Deshalb accel begrenzen:
    //** Die theoretisch maximale Beschl. ist max_force/mass. Zuerst
    //** begrenzen wir die Wunschbeschleunigung auf diesen Wert. Das ist
    //** sozusagen das, was das Triebwerk schaffen wuerde. Weiterhin
    //** haben wir eine Geschwindigkeit, die bremst (wegen f = force +
    //** air*v = m*a ergibt sich fuer a(res)=force/m - air*v/m, wobei
    //** force/m die schon korrigierte maximalbeschleunigung ist).
    //** -------------------------------------------------------------*/
    //accel_diff = nc_sqrt((bact->accel.x)*(bact->accel.x)+
    //                     (bact->accel.y)*(bact->accel.y)+
    //                     (bact->accel.z)*(bact->accel.z));
    //if( accel_diff > (bact->max_force/bact->mass) ) {
    //
    //    FLOAT rel = (bact->max_force/bact->mass)/accel_diff;
    //    bact->accel.x *= rel;
    //    bact->accel.y *= rel;
    //    bact->accel.z *= rel;
    //    }
    //bact->accel.x = bact->accel.x-bact->air_const*bact->dof.x*bact->dof.v/bact->mass;
    //bact->accel.y = bact->accel.y-bact->air_const*bact->dof.y*bact->dof.v/bact->mass;
    //bact->accel.z = bact->accel.z-bact->air_const*bact->dof.z*bact->dof.v/bact->mass;

    /*** ------------------  P O S I T I O N ---------------- ***/

    /* -----------------------------------------------------------
    ** Weil ich mich von pos auf die neue Position zubewege, merke
    ** ich mir diese in old_pos. Wenn meine Abweichung zu stark
    ** ist, korrigiere ich radikal, setze also die Position.
    ** Ansonsten verlasse ich mich auf die Geschwindigkeit.
    ** 1. Gucken, ob Abweichung pos zu den old_pos, welches ich
    **    hier eigentlich erreichen sollte, zu gross ist.
    ** 2. evtl. pos korrigieren
    ** 3. neue pos in old_pos merken, auf die wir im naechsten
    **    Netzframe zusteuern.
    ** ---------------------------------------------------------*/
    //if( nc_sqrt( (bact->pos.x-bact->old_pos.x)*(bact->pos.x-bact->old_pos.x)+
    //             (bact->pos.y-bact->old_pos.y)*(bact->pos.y-bact->old_pos.y)+
    //             (bact->pos.z-bact->old_pos.z)*(bact->pos.z-bact->old_pos.z))>30.0 )

    bact->pos = bact->old_pos;            // zurechtbiegen
    bact->old_pos   = p;

    /* -------------------------------------------------------------
    ** Positionen koennen sich ruckartig aendern (z.B. Robobeamen) 
    ** -----------------------------------------------------------*/
    if( vdm->block[ count ].i.specialinfo & SIF_SETDIRECTLY ) {

        bact->pos   = bact->old_pos;
        bact->dof.v = 0.0;
        }

    /*** -------------- A U S R I C H T U N G -------------- ***/

    /*** Matrix entpacken aus Eulerwinkel ***/
    ea.x = (((float)(vdm->block[ count ].i.roll))/127.0)*2*PI;
    ea.y = (((float)(vdm->block[ count ].i.pitch))/127.0)*2*PI;
    ea.z = (((float)(vdm->block[ count ].i.yaw))/127.0)*2*PI;
    ea.w = EulOrdXYZs;
    Eul_ToHMatrix(ea,hm);
    neu_dir.m11=hm[0][0]; neu_dir.m12=hm[0][1]; neu_dir.m13=hm[0][2];
    neu_dir.m21=hm[1][0]; neu_dir.m22=hm[1][1]; neu_dir.m23=hm[1][2];
    neu_dir.m31=hm[2][0]; neu_dir.m32=hm[2][1]; neu_dir.m33=hm[2][2];

    /* ------------------------------------------------------------------
    ** Differenz-Matrix ermitteln. Ebenso wie bei der Position gibt
    ** es die Möglichkeit, die Matrix zu korrigieren, was zu Rucklern
    ** führt, oder immer von der tatsächlichen Matrix auszugehen.
    ** Im exakten Weg gehe ich von old_dir nach new_dir und korrigiere,
    ** im unexakten, aber nicht ruckelnden Weg gehe ich vom tatsächlichen
    ** dir auf new_dir
    ** ----------------------------------------------------------------*/
    bact->d_matrix.m11 = (neu_dir.m11 - bact->old_dir.m11) / net_time;
    bact->d_matrix.m12 = (neu_dir.m12 - bact->old_dir.m12) / net_time;
    bact->d_matrix.m13 = (neu_dir.m13 - bact->old_dir.m13) / net_time;
    bact->d_matrix.m21 = (neu_dir.m21 - bact->old_dir.m21) / net_time;
    bact->d_matrix.m22 = (neu_dir.m22 - bact->old_dir.m22) / net_time;
    bact->d_matrix.m23 = (neu_dir.m23 - bact->old_dir.m23) / net_time;
    bact->d_matrix.m31 = (neu_dir.m31 - bact->old_dir.m31) / net_time;
    bact->d_matrix.m32 = (neu_dir.m32 - bact->old_dir.m32) / net_time;
    bact->d_matrix.m33 = (neu_dir.m33 - bact->old_dir.m33) / net_time;

    /*** Wenn d_matrix-Begrenzung, dann hier ***/
    
    /*** Matrix übernehmen ebenso wie Position ***/
    bact->dir     = bact->old_dir;
    bact->old_dir = neu_dir;

    /*** SpecialInfo ***/
    if( vdm->block[ count ].i.specialinfo & SIF_DONTRENDER )
        bact->ExtraState |= EXTRA_DONTRENDER;
    if( vdm->block[ count ].i.specialinfo & SIF_LANDED )
        bact->ExtraState |=  EXTRA_LANDED;
    else
        bact->ExtraState &= ~EXTRA_LANDED;

    /*** Zeit aktualisieren ***/
    bact->last_frame = timestamp;


    /*** ---------------------- E N E R G I E ----------------------- ***/
    bact->Energy = vdm->block[ count ].i.energy;
}



void yw_ReadVehicleData( struct Bacterium *bact,
                         struct ypaworld_data *ywd,
                         struct ypamessage_vehicledata_i *vdm,
                         ULONG  count, LONG timestamp )
{
    /*** Interpolate oder Extrapolate? ***/
    if( ywd->interpolate )
        yw_ReadVehicleData_I( bact, ywd, vdm, count, timestamp );
    else
        yw_ReadVehicleData_E( bact, ywd, (struct ypamessage_vehicledata_e *) vdm, count, timestamp );
}



void yw_ExtractVehicleData( struct ypaworld_data *ywd,
                            struct ypamessage_vehicledata_i *vdm,
                            struct OBNode *robo )
{
    /* ------------------------------------------------
    ** Wir lesen die Strukturen aus und suchen nach den
    ** Bacterien anhand des identifiers
    ** ----------------------------------------------*/
    ULONG  j;

    j = vdm->head.number;
    while( j ) {

        struct Bacterium *bact;
        ULONG  i;

        i = j - 1;

        bact = yw_GetBactByID( robo->bact, vdm->block[ i ].i.ident );

        if( !bact )
            yw_NetLog("+++ EVD: Haven't found vehicle ident %d  from owner %d (%dsec)\n",
                    vdm->block[ i ].i.ident , 
                    robo->bact->Owner, ywd->TimeStamp/1000);

        if( bact ) yw_ReadVehicleData( bact, ywd, vdm, i, ywd->TimeStamp );

        j--;
        }
}



#endif
