/*
**  $Source: $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:24:47 $
**  $Locker: $
**  $Author: $
**
**  Diverses...
**
**  (C) Copyright 1995 by Andreas Flemming
*/
#include <exec/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "engine/engine.h"
#include "transform/tform.h"
#include "baseclass/baseclass.h"
#include "ypa/ypaworldclass.h"
#include "ypa/ypabactclass.h"
#include "ypa/yparoboclass.h"
#include "input/input.h"

#ifdef __NETWORK__
#include "network/networkclass.h"
#include "ypa/ypamessages.h"
#endif


/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine

extern ULONG YPA_CommandCount;

/* Brot-O-Typen */
struct Bacterium *yb_SearchEnemy( struct Cell *sector, UBYTE owner );
void yb_SumForce( struct checkforcerelation_msg *cfr, struct Cell *sector,
                  struct ypabact_data *ybd );
struct Bacterium *yb_MakeCommandFromSlaves( struct Bacterium *bact, UBYTE k );
struct Bacterium *yb_GetBestChief( struct Bacterium *bact );
struct Bacterium *yb_GetNearest( struct Bacterium *bact );
void yb_SendSquadronStructure( struct ypaworld_data *ywd, UBYTE owner );
void yb_CopyWayPoints( struct Bacterium *from, struct Bacterium *to );
void yb_CopyTargetToSlave( struct Bacterium *com, struct Bacterium *slave );
void yb_CopyTarget( struct Bacterium *com, struct Bacterium *slave );   
void yb_TakeCommandersTarget( struct Bacterium *slave, struct Bacterium *chief, Object *world );
struct Bacterium *yb_SearchBact( Object *world, ULONG commandid, UBYTE owner );

/*** Arrays mach ma global ***/
FLOAT yb_delta[8][2] = { { -SECTOR_SIZE, -SECTOR_SIZE}, {0.0, -SECTOR_SIZE},
                         { SECTOR_SIZE, -SECTOR_SIZE}, {-SECTOR_SIZE, 0.0 },
                         { SECTOR_SIZE, 0.0}, {-SECTOR_SIZE, SECTOR_SIZE},
                         { 0.0, SECTOR_SIZE}, {SECTOR_SIZE, SECTOR_SIZE} };

/*** Wegen StackProblemen global jetzt ***/
struct ypamessage_neworg no;
 
_dispatcher( BOOL, yb_YBM_CRASHTEST, struct flt_triple *Plane)
{
/*
**  FUNCTION    Testet, ob dies ein harter Aufprall war, der zum Beispiel
**              eine Zerstörung des Objektes zur Folge haben kann.
**              Achtung, wir testen hier, ob der Aifprall hart war, also ob 
**              überhaupt Energie abgezogen werden darf und ob, wenn die
**              Energie niedrig war, das Objekt explodieren darf.
**              Somit verhindern wir, das bei jedem Scheiß was gemacht werden 
**              muß (was der Spieler vielleicht gar nicht als Kollision begreift!)
**
**              Übergeben wird
**              die Flächennormale. Ich multipliziere sie Skalar mit meiner
**              derzeitigen Geschwindigkeitsrichtung und vergleiche den
**              betrag des resultates mit CRASH_SPEED aus dem bactclass-
**              Include.
**
**  INPUT       Die Flächennormale
**
**  RESULT      TRUE für'n starken Aufprall
**
**  CHANGED     16-Oct-95   8100 000C created
*/

    struct ypabact_data *ybd;
    FLOAT  sp;

    ybd = INST_DATA(cl, o );

    sp = Plane->x * ybd->bact.dof.x * ybd->bact.dof.v +
         Plane->y * ybd->bact.dof.y * ybd->bact.dof.v +
         Plane->z * ybd->bact.dof.z * ybd->bact.dof.v;

    if( fabs( sp ) > CRASH_SPEED )
        return( TRUE );
    else
        return( FALSE );
}


_dispatcher( void, yb_YBM_CHECKFORCERELATION, struct checkforcerelation_msg *cfr)
{
/*
**  FUNCTION    Untersucht in einem gebiet (pos + umliegende Sektoren)
**              das Kräfteverhältnis. Dabei werden die bakterien aufaddiert
**              und der innere Sektor mit seiner Sektorenergie, wenn er feinlich
**              ist. Das kann man aber unterdrücken.
**
**  INPUT       position oder meine Position (--> CFR_MYPOS )
**
**  RESULT      my_energy, his_energy
**
**  CHANGED     8100 000C   25-Oct-95
*/


    struct ypabact_data *ybd;
    WORD   MapX, MapY;
    struct Cell *checksector, *sector;
    struct getsectorinfo_msg gsi;
    LONG   Energy, i, j;
    UBYTE  mask;

    ybd = INST_DATA(cl, o);

    /* Wie groß ist die Welt? (damit ich die Sektoren leichter ermitteln kann) */
    MapX = ybd->bact.WSecX;
    MapY = ybd->bact.WSecY;

    cfr->my_energy = cfr->his_energy = 0;

    if( cfr->flags & CFR_MYPOS ) {

        gsi.abspos_x = ybd->bact.pos.x;
        gsi.abspos_z = ybd->bact.pos.z;
        }
    else {

        gsi.abspos_x = cfr->pos.x;
        gsi.abspos_z = cfr->pos.z;
        }
    if( !_methoda( ybd->world, YWM_GETSECTORINFO, &gsi) )  return;

    sector = gsi.sector;
    mask   = 1 << ybd->bact.Owner;

    /*** Nun die Kräfte aufaddieren ***/

    if( (gsi.sec_x > 0) && (gsi.sec_y > 0) ) {

        checksector = &(sector[ -MapX - 1 ]);
        if( checksector->FootPrint & mask )
            yb_SumForce( cfr, checksector, ybd );
        }

    if( gsi.sec_y > 0 ) {

        checksector = &(sector[ -MapX ]);
        if( checksector->FootPrint & mask )
            yb_SumForce( cfr, checksector, ybd );
        }

    if( (gsi.sec_x < (MapX-1)) && (gsi.sec_y > 0) ) {

        checksector = &(sector[ -MapX + 1 ]);
        if( checksector->FootPrint & mask )
            yb_SumForce( cfr, checksector, ybd );
        }

    if( gsi.sec_x > 0 ) {

        checksector = &(sector[ -1 ]);
        if( checksector->FootPrint & mask )
            yb_SumForce( cfr, checksector, ybd );
        }

    if( sector->FootPrint & mask )
        yb_SumForce( cfr, sector, ybd );

    if( gsi.sec_x < (MapX-1) ) {

        checksector = &(sector[ 1 ]);
        if( checksector->FootPrint & mask )
            yb_SumForce( cfr, checksector, ybd );
        }

    if( (gsi.sec_x > 0) && (gsi.sec_y < (MapY-1)) ) {

        checksector = &(sector[ MapX - 1 ]);
        if( checksector->FootPrint & mask )
            yb_SumForce( cfr, checksector, ybd );
        }

    if( gsi.sec_y < (MapY-1) ) {

        checksector = &(sector[ MapX ]);
        if( checksector->FootPrint & mask )
            yb_SumForce( cfr, checksector, ybd );
        }

    if( (gsi.sec_x < (MapX-1)) && (gsi.sec_y < (MapY-1)) ) {

        checksector = &(sector[ MapX + 1 ]);
        if( checksector->FootPrint & mask )
            yb_SumForce( cfr, checksector, ybd );
        }

    /* ---------------------------------------------------------------------
    ** Sektorenergie, nur einen Teil davon. Alles unter 0 mißachte ich,
    ** damit bei teilweise zerschossenen 3x3-Sektoren keine falschen
    ** Informationen herauskommen.
    **
    ** Ich beachte nur den Sektor, von dem die Meldung kam, denn benachbarte
    ** Bakterien können mich angreifen, Sektoren nicht
    ** 0-Sektoren: keine Ausnahme mehr!
    ** -------------------------------------------------------------------*/

    Energy = 0;
    if( !(cfr->flags & CFR_NOSECTOR)) {

        if( sector->SType == SECTYPE_COMPACT ) {

            if( sector->SubEnergy[0][0] < 0 ) Energy = 0;
                else                          Energy = sector->SubEnergy[0][0];
            }
        else {

            for( i=0; i<3; i++ )
                for( j=0; j<3; j++ ) {

                    if( sector->SubEnergy[i][j] > 0 )
                        Energy += sector->SubEnergy[i][j];
                    }
            Energy /= 9;
            }

        /* --------------------------------------------------------------------
        ** Energy hat jetzt den Byte-Wert, muß also noch mit 250 multipliziert
        ** werden. Sektoren wehren sich aber nicht. So würde ich den Wert nicht 
        ** real nehmen, sondern etwas tiefer ansetzen (100?)
        ** ------------------------------------------------------------------*/
        Energy *= 120;

        /*** Nun noch zuweisen ***/
        if( sector->Owner == ybd->bact.Owner ) {

            /* nur aufaddieren, wenn es gewünscht wird */
            if( cfr->flags & CFR_MYSECTORENERGY )
                cfr->my_energy  += Energy;
            }
        else
            cfr->his_energy += Energy;
        }
}


void yb_SumForce( struct checkforcerelation_msg *cfr, struct Cell *sector,
                  struct ypabact_data *ybd )
{

   /* ----------------------------------------------------------------------
   ** Summiert die Kräfteverhältnisse im angegebenen Sector. Nur Bacterien!
   ** Der Robo wird hier nicht beachtet!
   ** --------------------------------------------------------------------*/

   struct Bacterium *kandidat;

   kandidat = (struct Bacterium *) sector->BactList.mlh_Head;
   while( kandidat->SectorNode.mln_Succ ) {

       if( ( kandidat->Owner       == 0 )                   ||

           ( kandidat->MainState   == ACTION_DEAD )         ||
           
           ((kandidat->BactClassID == BCLID_YPAROBO ) &&
            (kandidat->Owner       == ybd->bact.Owner) )    ||
           
           ( kandidat->BactClassID == BCLID_YPAMISSY ) ) {
           kandidat = (struct Bacterium *) kandidat->SectorNode.mln_Succ;
           continue;
           }

        if( kandidat->Owner == ybd->bact.Owner )
            cfr->my_energy  += kandidat->Energy;
        else
            cfr->his_energy += kandidat->Energy;

        kandidat = (struct Bacterium *) kandidat->SectorNode.mln_Succ;
        }


}



_dispatcher( void, yb_YBM_GIVEENEMY, struct giveenemy_msg *ge )
{
/*
**  FUNCTION    sucht in 9 Sektoren, deren mittlerster durch die Position
**              übergeben wurde, nach feinden und liefert die Informationen
**              darüber zurück. Wenn sich dort 2 geschwader aufhalten, wird nur
**              das erste, was wir finden, beachtet. Das bringt dann im
**              Spiel die Flemmingsche Unschärferelation...
**
**  INPUT       Position (eigene oder Extra, siehe Flags)
**
**  RESULTS     Objekt und Bakterium des feindlichen Geschwaderführers,
**              seine CommandID und sein Eigentümer
**
**  CHANGED     8100 000C 25-Oct-95
*/


    struct Bacterium *opfer = NULL, *bact;
    struct getsectorinfo_msg gsi;
    struct ypabact_data *ybd;
    FLOAT  mpx, mpy;
    int i;


    ge->CommandID = ge->owner  = 0;
    ge->mbact     = NULL;
    ge->master    = NULL;
    ybd = INST_DATA( cl, o );

    if( ge->flags & GE_MYPOS ) {

        gsi.abspos_x = ybd->bact.pos.x;
        gsi.abspos_z = ybd->bact.pos.z;
        }
    else {

        gsi.abspos_x = ge->pos.x;
        gsi.abspos_z = ge->pos.z;
        }
    if( !_methoda( ybd->world, YWM_GETSECTORINFO, &gsi) )   return;


    mpx =  ((FLOAT)gsi.sec_x + 0.5) * SECTOR_SIZE;
    mpy = -((FLOAT)gsi.sec_y + 0.5) * SECTOR_SIZE;

    if( !(opfer = yb_SearchEnemy( gsi.sector, ybd->bact.Owner )) ) {

        /* Wir konnten im eigenen Sektor nichts finden. Testen wir mal die
        ** anderen Sektoren */
        for( i=0; i<8; i++ ) {

            gsi.abspos_x = mpx + yb_delta[ i ][ 0 ];
            gsi.abspos_z = mpy + yb_delta[ i ][ 1 ];
            if( _methoda( ybd->world, YWM_GETSECTORINFO, &gsi )) {

                /* Da ist ein Sector, testen wir den mal */
                if( opfer = yb_SearchEnemy( gsi.sector, ybd->bact.Owner))
                    break;
                }
            }
        }

    if( opfer ) {

        /*
        ** Wir haben jemand! Sag uns deinen Chef oder bist du selber einer? 
        */

        if( opfer->master == opfer->robo ) {
            ge->master    = opfer->BactObject;
            ge->mbact     = opfer;
            ge->CommandID = opfer->CommandID;
            ge->owner     = opfer->Owner;
            }
        else {
            _get( opfer->master, YBA_Bacterium, &bact );
            ge->master    = opfer->master;
            ge->mbact     = bact;
            ge->CommandID = bact->CommandID;
            ge->owner     = bact->Owner;
            }
        }
}


struct Bacterium *yb_SearchEnemy( struct Cell *sector, UBYTE owner )
{
    /*
    ** Sucht einfach bloß einen Gegner, der lebt und keine Rakete ist
    */

    struct Bacterium *kandidat = NULL;

    kandidat = (struct Bacterium *) sector->BactList.mlh_Head;
    while( kandidat->SectorNode.mln_Succ ) {

        if( (kandidat->BactClassID != BCLID_YPAMISSY )   &&
            (!(kandidat->ExtraState & EXTRA_LOGICDEATH)) &&
            (kandidat->Owner != 0 )                      &&
            (kandidat->Owner != owner ) ) {

            return( kandidat );
            }

        kandidat = (struct Bacterium *) kandidat->SectorNode.mln_Succ;
        }

    return( NULL );
}


_dispatcher( void, yb_YBM_MERGE, Object *slave )
{
/*
**  FUNCTION    Slave will sich mit all seinen Untergebenen beei mir anmelden.
**              Dazu klinke ich ihn bei mir ein, doch vorher schnappe ich
**              mir noch all seine Untergebenen. Dann wende ich noch mein
**              Aggressivitäts-set auf mich an.
**
**  INPUT       Der neue
**
**  RESULT      nix, wieso?
**
**  CHANGED     heute erstmal geschrieben
*/
    struct ypabact_data *ybd;
    struct MinList *list;
    struct OBNode *jemand;
    struct Bacterium *slavebact;

    ybd = INST_DATA( cl, o );


    _get( slave, YBA_Bacterium, &slavebact );
    list = &(slavebact->slave_list);

    /*** seine slaves als meine slaves ***/
    while( jemand = (struct OBNode *) list->mlh_Head )
        _methoda( o, YBM_ADDSLAVE, jemand->o );

    /*** nun noch er selbst ***/
    _methoda( o, YBM_ADDSLAVE, slave );

    /*** neue Leute instruieren ***/
    _set( o, YBA_Aggression, (LONG) ybd->bact.Aggression );
}



_dispatcher( void, yb_YBM_HANDLEVISCHILDREN, struct handlevischildren_msg *hvc )
{
/*
**  FUNCTION    Hat die Aufgabe, etwas mit den Kindern der Visprotos zu tun,
**              nämlich sie anzukoppeln oder abzukoppeln.
**
**  INPUT       art, also was gemacht werden soll
**
**  OUTPUT      put put put mein Hühnchen, put put put mein Hahn,
**              möchte gerne wissen, wer Eier legen kann.
**              Eier legt die Köchin, Eier legt der Koch,
**              Eier legt der Koch der Köchin vor das ...
**
**  CHANGED     27-feb-96   8100 000C   created
*/

    struct MinList *childlist;
    struct ObjectNode *child;
    Object *vp[7];
    int    i;
    struct flt_vector_msg pos;
    struct ypabact_data *ybd;
    FLOAT  iks, yps, zett;


    ybd = INST_DATA( cl, o);

    vp[0] = ybd->bact.vis_proto_normal;
    vp[1] = ybd->bact.vis_proto_dead;
    vp[2] = ybd->bact.vis_proto_fire;
    vp[3] = ybd->bact.vis_proto_create;
    vp[4] = ybd->bact.vis_proto_wait;
    vp[5] = ybd->bact.vis_proto_megadeth;
    vp[6] = NULL;

    i = 0;
    while( vp[ i ] ) {

        /*** Liste aller Kinder holen ***/
        _get( vp[i], BSA_ChildList, &childlist );

        /*** Nun die Kinder je nach Wunsch bearbeiten ***/
        child = (struct ObjectNode *) childlist->mlh_Head;
        while( child->Node.mln_Succ ) {

            switch( hvc->job ) {

                case HVC_CONNECTCHILDREN:

                    /*** FollowMother = TRUE und pos relativ ***/
                    _set( child->Object, BSA_FollowMother, TRUE );

                    _get( child->Object, BSA_X, &iks);
                    _get( child->Object, BSA_Y, &yps);
                    _get( child->Object, BSA_Z, &zett);

                    /*** Position wieder relativ zu mir ***/
                    pos.x = iks  - ybd->bact.pos.x;
                    pos.y = yps  - ybd->bact.pos.y;
                    pos.z = zett - ybd->bact.pos.z;
                    pos.mask = VEC_X | VEC_Y | VEC_Z;
                    _methoda( child->Object, BSM_POSITION, &pos );

                    break;

                case HVC_REMOVECHILDREN:

                    /*** FollowMother = FALSE und pos relativ ***/
                    _set( child->Object, BSA_FollowMother, TRUE );

                    _get( child->Object, BSA_X, &iks);
                    _get( child->Object, BSA_Y, &yps);
                    _get( child->Object, BSA_Z, &zett);

                    /*** Position wieder relativ zu mir ***/
                    pos.x = iks  + ybd->bact.pos.x;
                    pos.y = yps  + ybd->bact.pos.y;
                    pos.z = zett + ybd->bact.pos.z;
                    pos.mask = VEC_X | VEC_Y | VEC_Z;
                    _methoda( child->Object, BSM_POSITION, &pos );

                    break;
                }

            /*** näxtes ***/
            child = (struct ObjectNode *) child->Node.mln_Succ;
            }
        
        /*** erst hier.... ***/
        i++;
        }
}


void yb_SendCommand( struct ypabact_data *ybd, struct Bacterium *bact, ULONG modus )
{
    struct ypaworld_data *ywd;

    /* ---------------------------------------------------------
    ** Versendet eine NewOrg-message mit dem geschwader von bact
    ** Der Modus wird nur zu debug-Zwecken mit übergeben
    ** -------------------------------------------------------*/
    ywd = INST_DATA( ((struct nucleusdata *)ybd->world)->o_Class, ybd->world);
    if( ywd->playing_network ) {

        struct sendmessage_msg sm;
        struct OBNode *s;

        no.basic.num_slaves = 0;
        no.basic.commander  = bact->ident;
        no.basic.command_id = bact->CommandID;

        /*** Seine Slaves ***/
        s = (struct OBNode *) bact->slave_list.mlh_Head;
        while( s->nd.mln_Succ ) {

            if( no.basic.num_slaves < MAXNUM_NEWORGSLAVES ) {
                no.slaves[ no.basic.num_slaves ] = s->bact->ident;
                no.basic.num_slaves++;
                }
            s = (struct OBNode *) s->nd.mln_Succ;
            }

        no.generic.message_id = YPAM_NEWORG;
        no.generic.owner      = ybd->bact.Owner;
        no.basic.modus        = modus;

        no.basic.size         = sizeof( struct ypamessage_generic ) +
                                sizeof( struct ypamessage_neworg_basic ) +
                                sizeof( ULONG ) * no.basic.num_slaves;

        sm.data               = &no;
        sm.data_size          = no.basic.size;
        sm.receiver_kind      = MSG_ALL;
        sm.receiver_id        = NULL;
        sm.guaranteed         = TRUE;
        _methoda( ywd->world, YWM_SENDMESSAGE, &sm );
        }
}


_dispatcher( void, yb_YBM_ORGANIZE, struct organize_msg *org )
{
/*
**  FUNCTION    Sorgt dafür, daß mit dem Objekt, auf welches die methode
**              angewendet wird, etwas gemacht wird. Entweder es wird
**              Geschwaderführer oder geht in ein anderes geschwader auf.
**
**  ACHTUNG     Diese Funktion wühlt in den Listen rum, also nicht in Schleifen
**              verwenen, wo ich auf Nachfolger angewiesen bin etc.
**
**  INPUT       Mode und evtl. weiteres bacterium
**              ORG_NEWCHIEF: specialbact ist das bacterium des neuen Chefs
**              ORG_BECOMECHIEF: specialbact ist chef des Geschwaders, welches
**                               ich übernehmen soll. Ist es 0, werde ich einfach 
**                               ein neuer Chef.
**
**  CHANGED     Das soll der Frühling sein ???
*/

    struct ypabact_data *ybd;
    struct settarget_msg target;
    struct MinList *list;
    ULONG  cc;
    struct Bacterium *newchief, *master;

    ybd = INST_DATA( cl, o);

    switch( org->mode ) {

        case ORG_NEWCHIEF:

            /* -----------------------------------------------------------------
            ** Ich bekomme einen neuen Chef. Ich klinke mich in das geschwader
            ** des neuen ein und, wenn ich selbst ein Chef war, auch alle
            ** meine Untergebenen. Das Geschwader, so ich Chef war, wird
            ** aufgelöst. Das heißt auch HauptZielabmeldung, löschen der ID usw.
            ** Alles neue kommt vom neuen Chef.
            ** ---------------------------------------------------------------*/

            if( org->specialbact == NULL ) return;

            if( ACTION_DEAD == org->specialbact->MainState ) {
                _LogMsg("ORG_NEWCHIEF: Dead master\n");
                return;
                }

            /*** Soll ich mir selbst übergeben werden? ***/
            if( (ybd->bact.master == org->specialbact->BactObject) ||
                (&(ybd->bact)     == org->specialbact) )
                return;

            /*** ID übernehmen (muß ich immer machen) ***/
            ybd->bact.CommandID = org->specialbact->CommandID;;

            /*** Aggression vom neuen Chef ***/
            ybd->bact.Aggression = org->specialbact->Aggression;

            /*** An neuen Chef hängen ***/
            _methoda( org->specialbact->BactObject, YBM_ADDSLAVE, o );

            /*** Slave noch umschichten ***/
            while (ybd->bact.slave_list.mlh_Head->mln_Succ) {

                struct OBNode *slave;

                slave = (struct OBNode *) ybd->bact.slave_list.mlh_Head;

                slave->bact->Aggression = org->specialbact->Aggression;
                slave->bact->CommandID  = org->specialbact->CommandID;
                
                /*** Ziel des Commanders uebernehmen ***/
                yb_TakeCommandersTarget( slave->bact, org->specialbact, ybd->world ); 

                /*** Umschichten ***/
                _methoda( org->specialbact->BactObject, YBM_ADDSLAVE, slave->o );
            };
                            
            /*** Ziel des Commanders uebernehmen ***/
            yb_TakeCommandersTarget( &(ybd->bact), org->specialbact, ybd->world ); 

            /* -----------------------------------------------------------
            ** org->specialbact wird Commander. Slaves sind seine Slaves,
            ** ich und meine Slaves. Aber die hängen ja alle schon drunter
            ** ---------------------------------------------------------*/
            yb_SendCommand( ybd, org->specialbact, ORG_NEWCHIEF );

            break;

        case ORG_BECOMECHIEF:

            /* -------------------------------------------------------------
            ** Das Objekt soll Chef werden. Wenn es schon Chef ist UND
            ** es das eigene geschwader übernehmen soll, breche ich ab.
            ** Ansonsten hole ich mir vom derzeitigen Chef Hauptziel und ID,
            ** mache mich zum Chef und hole mir dann alle leute, egal, ob 
            ** ich schon welche habe. Wenn ich Chef war und auf einen Chef
            ** angewendet werde, dann übernehme ich zusätzlich seine Leute,
            ** aber auch sein Ziel und seine ID. Wenn kein Pointer übergeben 
            ** wurde, dann werde ich ein neues geschwader, solange ich
            ** kein Commander war (sonst hat es ja keinen Sinn)
            ** -----------------------------------------------------------*/

            if( (ybd->bact.master  == ybd->bact.robo) &&
                (&(ybd->bact) == org->specialbact) )
                return;

            if( ACTION_DEAD == ybd->bact.MainState ) {
                _LogMsg("ORG_BECOMECHIEF dead vehicle\n");
                return;
                }

            /*** Ich werde zum Chef ***/
            if( ybd->bact.master != ybd->bact.robo )
                _methoda( ybd->bact.robo, YBM_ADDSLAVE, o );

            /*** Wenn specialbact != NULL, dann alle diese zusätzlich einklinken ***/
            if( org->specialbact ) {

                /*** Ziel übernehmen ***/
                yb_TakeCommandersTarget( &(ybd->bact), org->specialbact, ybd->world );

                /*** Aggression übernehmen ***/
                ybd->bact.Aggression = org->specialbact->Aggression;

                /*** CommandID übernehmen ***/
                ybd->bact.CommandID  = org->specialbact->CommandID;

                /*** specialbact und slaves übernehmen ***/
                _methoda( o, YBM_ADDSLAVE, org->specialbact->BactObject );
                list = &( org->specialbact->slave_list );
            
                /*** Slave noch umschichten ***/
                while (list->mlh_Head->mln_Succ) {

                    struct OBNode *slave;

                    slave = (struct OBNode *) list->mlh_Head;

                    /*** Umschichten ***/
                    _methoda( o, YBM_ADDSLAVE, slave->o );

                    slave->bact->Aggression = org->specialbact->Aggression;
                    
                    /*** Ziel ubernehmen ***/
                    yb_TakeCommandersTarget( slave->bact, &(ybd->bact), ybd->world );
                };


                /*** CommandID übernehmen ***/
                ybd->bact.CommandID = org->specialbact->CommandID;
                }
            else {

                /*** Hoppla, ich werde Chef eines vollkommen neuen Geschwaders ***/

                if( ybd->bact.master != ybd->bact.robo ) {

                    _get( ybd->bact.robo, YRA_CommandCount, &cc );
                    ybd->bact.CommandID  = cc;
                    ybd->bact.CommandID |= (((ULONG)ybd->bact.Owner) << 24);
                    cc++;
                    _set( ybd->bact.robo, YRA_CommandCount, cc );
                    }
                }

            yb_SendCommand( ybd, &(ybd->bact), ORG_BECOMECHIEF );

            break;

        case ORG_NEWCOMMAND:

            /* -------------------------------------------------------
            ** Ich werde Chef eines vollkommen neuen Geschwaders. Wenn
            ** ich Slave bin, dann löse ich mich raus, andernfalls
            ** mache ich aus meinen Slaves ein neues geschwader. In
            ** Jedem Fall wird der CommandCount hochgezählt.
            ** -----------------------------------------------------*/

            if( ACTION_DEAD == ybd->bact.MainState ) {
                _LogMsg("ORG_NEWCOMMAND: dead vehicle\n");
                return;
                }

            newchief = NULL;
            if( ybd->bact.master == ybd->bact.robo ) {

                /*** Bin Commander. Löse mich heraus ***/
                newchief = yb_MakeCommandFromSlaves( &(ybd->bact), 0 );

                #ifdef __NETWORK__
                /*** Hier entstand (VIELLEICHT) ein vollkommen neues Geschwader ***/
                if( newchief )
                    yb_SendCommand( ybd, newchief, ORG_NEWCOMMAND+10 );
                #endif
                }
            else {

                /*** Einklinken ***/
                _methoda( ybd->bact.robo, YBM_ADDSLAVE, ybd->bact.BactObject);
                
                /*** Ziel nicht löschen ***/
                }

            _get( ybd->bact.robo, YRA_CommandCount, &cc );
            ybd->bact.CommandID  = cc;
            ybd->bact.CommandID |= (((ULONG)ybd->bact.Owner) << 24);
            cc++;
            _set( ybd->bact.robo, YRA_CommandCount, cc );

            /*** ich bin jetzt neues geschwader ***/
            yb_SendCommand( ybd, &(ybd->bact), ORG_NEWCOMMAND );

            break;

        case ORG_ADDSLAVE:

            /* -----------------------------------------------------
            ** Ich (also ybd) bin Chef und bekomme in specialbact
            ** einen neuen Slave. Ist der neue ein Commander, so
            ** muß er sich von seinem alten geschwader verabschieden
            ** und die wählen einen neuen Chef. Er nimmt seine
            ** Leute NICHT mit!
            ** ---------------------------------------------------*/

            master = NULL;
            if( org->specialbact->master == org->specialbact->robo ) {

                /*** Der neue ist ein Commander, Slaves abkoppeln ***/
                master = yb_MakeCommandFromSlaves( org->specialbact, 0 );

                /*** Hier entstand (VIELLEICHT) ein neues geschwader ***/
                if( master )
                    yb_SendCommand( ybd, master, ORG_ADDSLAVE+10 );
                }

            /*** Ankoppeln ***/
            _methoda( o, YBM_ADDSLAVE, org->specialbact->BactObject );

            /*** CommandID nachtragen ***/
            org->specialbact->CommandID = ybd->bact.CommandID;
            
            /*** to this slave only ***/
            yb_TakeCommandersTarget( org->specialbact, &(ybd->bact), ybd->world );

            yb_SendCommand( ybd, &(ybd->bact), ORG_ADDSLAVE );

            break;

        case ORG_NEARTOCOM:

            /* ----------------------------------------------------------
            ** Ich gebe die Verantwortung ab, mache also einen aus meinem
            ** Geschwader zum neuen Chef. Dabei entscheidet die Zielnähe
            ** und sonst nix. USER WERDEN IGNORIERT!
            ** Keine Parameter.
            ** Der Wechsel wird erzwungen, auch wenn der Comm. der gün-
            ** stigste ist.
            ** --------------------------------------------------------*/

            /*** Finden ***/
            _get( o, YBA_UserInput, &cc );
            if( !cc ) {

                struct Bacterium *chef;
                struct ypaworld_data *ywd;

                chef = yb_MakeCommandFromSlaves( &(ybd->bact), 1 );

                if( chef ) {

                    /*** mich selbst dort ankoppeln ***/
                    _methoda( chef->BactObject, YBM_ADDSLAVE, o );
                    chef->CommandID = ybd->bact.CommandID;

                    /*** Chef ist der neue ***/
                    yb_SendCommand( ybd, chef, ORG_NEARTOCOM );
                    }
                }

            break;

        }
}


struct Bacterium *yb_MakeCommandFromSlaves( struct Bacterium *bact, UBYTE k )
{
    /* --------------------------------------------------------
    ** Aus meinen Slaves wird ein neues geschwader gemacht. Das
    ** heißt, denen muß mein Hauptziel übergeben werden und der
    ** CommandCount wird ebenfalls mit übergeben. Es wird
    ** sozusagen der bisherige Chef herausgelöst. Ob der dann
    ** ein neues geschwader wird, ist hier egal.
    **
    ** Für das Netzwerk reicht eine NEWCHIEF-Message
    ** ------------------------------------------------------*/

    if( bact->slave_list.mlh_Head->mln_Succ ) {

        /* -------------------------------------------------
        ** es gibt tatsächlich untergebene. Wer ist denn der
        ** beste Kandidat für den ChefPosten ?
        ** -----------------------------------------------*/
        struct Bacterium *newchief;
        struct settarget_msg target;
        struct OBNode *slave;
        Object  *world;
        
        _get( bact->BactObject, YBA_World, &world );

        if( k == 0 )
            newchief = yb_GetBestChief( bact );
        else
            newchief = yb_GetNearest( bact );

        if( !newchief ) return( NULL );

        /* --------------------------------------------------------
        ** Wenn der neue ein UFO ist, der alte aber keins, so haben
        ** wir keinen besseren gefunden
        ** ------------------------------------------------------*/
        if( (BCLID_YPAUFO == newchief->BactClassID) &&
            (BCLID_YPAUFO != bact->BactClassID) )
            return( NULL );

        /*** Unter dem Robo einklinken ***/
        _methoda( bact->robo, YBM_ADDSLAVE, newchief->BactObject);
        
        /*** Der neue bekommt mein Ziel ***/
        yb_TakeCommandersTarget( newchief, bact, world );

        /* ----------------------------------------------------
        ** jetzt ist der neue raus und alle, die noch unter mir
        ** hängen, werden dem neuen übergeben
        ** --------------------------------------------------*/
        slave = (struct OBNode *) bact->slave_list.mlh_Head;
        while( slave->nd.mln_Succ ) {

            struct OBNode *next_slave = (struct OBNode *) slave->nd.mln_Succ;
            _methoda( newchief->BactObject, YBM_ADDSLAVE, slave->o );
            
            /*** nimm das Ziel des Commanders ***/
            yb_TakeCommandersTarget( slave->bact, newchief, world );
            
            slave = next_slave;
            }

        /*** Was fehlt noch? Das Ziel! ***/

        newchief->CommandID = bact->CommandID;

        /*** Chef zurück ***/
        return( newchief );
        }
    else {

        /*** is ja keiner da ***/
        return( NULL );
        }
}


struct Bacterium *yb_GetBestChief( struct Bacterium *bact )
{
    /* -------------------------------------------------------
    ** Sucht neuen Commander. Der muß nicht besser als der
    ** jetzige sein! Auswahlkriterien sind geringe Entfernung,
    ** Energie und UFO-Freiheit.
    ** -----------------------------------------------------*/

    struct OBNode *slave, *merk_slave = NULL;
    FLOAT  merk_value = 0.0;

    slave = (struct OBNode *) bact->slave_list.mlh_Head;
    while( slave->nd.mln_Succ ) {

        FLOAT value;

        /*** Leichen werden nicht mehr befördert ***/
        if( ACTION_DEAD == slave->bact->MainState ) {

            slave = (struct OBNode *) slave->nd.mln_Succ;
            continue;
            }

        if( BCLID_YPAUFO == slave->bact->BactClassID )
            value = 0.0;
        else {

            /*** es lohnt eine genauere Untersuchung ***/
            value = nc_sqrt( (bact->pos.x - slave->bact->pos.x) *
                             (bact->pos.x - slave->bact->pos.x) +
                             (bact->pos.z - slave->bact->pos.z) *
                             (bact->pos.z - slave->bact->pos.z));
            value = 1 - value / (92 * SECTOR_SIZE);

            value += ( ((FLOAT)slave->bact->Energy) /
                       ((FLOAT)slave->bact->Maximum) );
            }

        if( (!merk_slave) || (merk_value < value) ) {

            merk_value = value;
            merk_slave = slave;
            }

        slave = (struct OBNode *) slave->nd.mln_Succ;
        }

    if( merk_slave )
        return( merk_slave->bact );
    else
        return( NULL );
}


struct Bacterium *yb_GetNearest( struct Bacterium *bact )
{
    /* ----------------------------------------------------
    ** Ermittelt aus den Slaves des "bact" den Zielnächsten
    ** und gibt ihn zurück
    ** --------------------------------------------------*/

    struct OBNode *knd;
    struct Bacterium *merke_knd;
    struct flt_triple tpos;
    FLOAT  distance, merke_distance;

    /*** Den nächstliegenden Slave suchen ***/
    if( TARTYPE_SECTOR == bact->PrimTargetType )
        tpos = bact->PrimPos;
    else {
        if( TARTYPE_BACTERIUM == bact->PrimTargetType )
            tpos = bact->PrimaryTarget.Bact->pos;
        else
            return( NULL ); // weil ???
        }

    merke_distance = 128 * 1.4 * SECTOR_SIZE; // ist größtmögliche Entfernung
    merke_knd = NULL;

    knd = (struct OBNode *) bact->slave_list.mlh_Head;
    while( knd->nd.mln_Succ ) {

        ULONG UI;

        /* --------------------------------------------------------------
        ** Leichen werden nicht mehr befördert. Das mag jetzt ziemlich
        ** hart klingen, aber es bringt doch definitiv Schwierigkeiten,
        ** wenn der Anführer eines geschwaders tot ist und so seinen
        ** Aufgaben nicht nachkommen kann. Ich denke, daß alle Anwesenden
        ** und Verwesenden für diese Maßnahme Verständnis aufbringen
        ** werden.
        ** ------------------------------------------------------------*/
        if( ACTION_DEAD == knd->bact->MainState ) {

            knd = (struct OBNode *) knd->nd.mln_Succ;
            continue;
            }

        _get( knd->o, YBA_UserInput, &UI );
        if( !UI ) {

            distance = nc_sqrt( (tpos.x - knd->bact->pos.x) *
                                (tpos.x - knd->bact->pos.x) +
                                (tpos.z - knd->bact->pos.z) *
                                (tpos.z - knd->bact->pos.z));

            /* -----------------------------------------------------------
            ** UFOs sollten nicht genommen werden, es kann aber passieren,
            ** daß nur noch UFOs da sind. Deshalb beachte ich sie erstmal
            ** und lasse NichtUFOs wichtiger erscheinen, wenn ein UFO
            ** vorher gemerkt wurde.
            ** Ich merke es,
            **     o wenn noch nichts gemerkt wurde
            **     o wenn es näherliegend und kein UFO ist
            **     o Wenn das vorherige ein UFO war und das jetzt kein
            **       UFO oder ein näherliegendes UFO ist
            ** ---------------------------------------------------------*/
            if( (merke_knd    == NULL) ||

               ((BCLID_YPAUFO != knd->bact->BactClassID) &&
                (merke_distance > distance)) ||

                ((BCLID_YPAUFO == merke_knd->BactClassID) &&
                ((BCLID_YPAUFO != knd->bact->BactClassID) ||
                 (merke_distance > distance))) ) {

                merke_distance = distance;
                merke_knd      = knd->bact;
                }
            }

        knd = (struct OBNode *) knd->nd.mln_Succ;
        }

    return( merke_knd );
}


void yb_CopyTarget( struct Bacterium *com, struct Bacterium *slave )
{
    /* -------------------------------------------------------------------
    ** Copies commanders target to the slave. Is there no slave given, set
    ** targets to alle slaves. targets include also waypoints.
    ** -----------------------------------------------------------------*/
    if( slave ) {
    
        /*** ONE SLAVE ONLY ***/
        yb_CopyTargetToSlave( com, slave );
        }
    else {
    
        /*** for every slave ***/
        struct OBNode *sl;
        sl = (struct OBNode *) com->slave_list.mlh_Head;
        while( sl->nd.mln_Succ ) {
        
            yb_CopyTargetToSlave( com, sl->bact );
            sl = (struct OBNode *) sl->nd.mln_Succ;
            }
        }
}            
         
    
void yb_CopyTargetToSlave( struct Bacterium *com, struct Bacterium *slave )   
{

    /*** gives target including waypoints to the selected slave ***/
    struct settarget_msg target;
    
    target.target.bact = com->PrimaryTarget.Bact;
    target.target_type = com->PrimTargetType;
    target.pos         = com->PrimPos;
    target.priority    = 0;
    _methoda( slave->BactObject, YBM_SETTARGET, &target );

    slave->num_waypoints      = com->num_waypoints;
    slave->count_waypoints    = 0; // damit er am Anfang anfängt!!
    memcpy( slave->waypoint, com->waypoint,
            sizeof( slave->waypoint ) );
    if( com->ExtraState & EXTRA_DOINGWAYPOINT ) {

        /*** Dann ersten Wegpunkt als Ziel setzen ***/
        slave->ExtraState |= EXTRA_DOINGWAYPOINT;
        if( com->ExtraState & EXTRA_WAYPOINTCYCLE )
            slave->ExtraState |= EXTRA_WAYPOINTCYCLE;
        target.pos.x       = slave->waypoint[ 0 ].x;
        target.pos.z       = slave->waypoint[ 0 ].z;
        target.target_type = TARTYPE_SECTOR;
        target.priority    = 0;
        _methoda( slave->BactObject, YBM_SETTARGET, &target );
        }
}


void yb_TakeCommandersTarget( struct Bacterium *slave, struct Bacterium *chief, Object *world )
{
/* ------------------------------------------------------------------------------
** So, das ist wieder mal ne Routine, die Ziele von einem Vehicle zum anderen
** schaufelt und hoffentlich die letzte ihrer Art. 
** Der Slave soll das Ziel des Commanders nehmen. Aber nicht einfach uebernehmen,
** weil das bei Panzern haeufig probleme bringt.
** Wir ermitteln das Ziel des chiefs, das kann sein:
**      PrimSektor, PrimBact oder letzter Wegpunkt oder gemerktes BactZiel
**      fuer Wegpunkte.
** Dann geben wir das Ziel weiter:
**      Als Wegpunktliste fuer Panzer und normal fuer den ganzen Rest.
** ---------------------------------------------------------------------------*/


    struct Bacterium *tbact = NULL;
    struct flt_triple tpos;
    ULONG   ttype;
    
    /*** erstmal alles aufraeumen! ***/
    slave->ExtraState   &= ~(EXTRA_DOINGWAYPOINT|EXTRA_WAYPOINTCYCLE);
    slave->num_waypoints = 0;
    
    /*** Was ist das Ziel ***/
    if( chief->ExtraState & EXTRA_DOINGWAYPOINT ) {
    
        /*** ist das "Ende" ein Geschwader? ***/
        if( chief->mt_commandid ) {
        
            /*** BactZiel ***/
            if( tbact = yb_SearchBact( world, chief->mt_commandid, chief->mt_owner ) ) 
                ttype = TARTYPE_BACTERIUM;
            else
                ttype = TARTYPE_NONE;
            }
        else {
        
            /*** SectorZiel ***/
            tpos.x = chief->waypoint[ chief->num_waypoints - 1 ].x;
            tpos.z = chief->waypoint[ chief->num_waypoints - 1 ].z;
            ttype  = TARTYPE_SECTOR;
            }
        }
    else {
    
        if( TARTYPE_BACTERIUM == chief->PrimTargetType ) {
            tbact = chief->PrimaryTarget.Bact;
            ttype = TARTYPE_BACTERIUM;
            }
        else {
        
            if( TARTYPE_SECTOR == chief->PrimTargetType ) {
                tpos.x = chief->PrimPos.x;
                tpos.z = chief->PrimPos.z;
                ttype  = TARTYPE_SECTOR;
                }
            else {
            
                ttype = TARTYPE_NONE;
                }
            }
        }
        
    /*** Wenn kein Ziel dann wenigstens den Commander als Ziel nehmen ***/
    if( TARTYPE_NONE == ttype ) {
    
        ttype = TARTYPE_BACTERIUM;
        tbact = chief;
        }
        
    /*** Nun Ziele zuweisen ***/
    if( (BCLID_YPATANK == slave->BactClassID) ||
        (BCLID_YPACAR  == slave->BactClassID) ) {
        
        struct findpath_msg fpath;
        
        /*** Bodenvehicle ***/
        if( TARTYPE_BACTERIUM == ttype ) {
            fpath.to_x  = tbact->pos.x;
            fpath.to_z  = tbact->pos.z;
            }
        else {
            fpath.to_x  = tpos.x;
            fpath.to_z  = tpos.z;
            }
            
        fpath.num_waypoints = MAXNUM_WAYPOINTS;
        fpath.from_x        = slave->pos.x;
        fpath.from_z        = slave->pos.z;
        fpath.flags         = WPF_Normal;
        
        _methoda( slave->BactObject, YBM_SETWAY, &fpath );

        if( TARTYPE_BACTERIUM == ttype ) {
        
            slave->mt_commandid = tbact->CommandID;
            slave->mt_owner     = tbact->Owner;
            }
        }
    else {
    
        /*** Luftvehicle. Ziel direkt setzen ***/
        struct settarget_msg target;
        
        target.target_type = ttype;
        target.priority    = 0;
        
        if( TARTYPE_BACTERIUM == ttype ) {
            target.target.bact = tbact;
            }
        else {
            target.pos.x = tpos.x;
            target.pos.z = tpos.z;
            }   
            
        _methoda( slave->BactObject, YBM_SETTARGET, &target ); 
        }
}    
