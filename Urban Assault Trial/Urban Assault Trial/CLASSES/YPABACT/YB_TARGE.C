/*
**  $Source: PRG:MovingCubesII/Classes/_YPABactClass/yb_user.c,v $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:24:47 $
**  $Locker: floh $
**  $Author: floh $
**
**  Steuerung des Objects durch User.
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


/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine

ULONG yb_DoingWaypoint( struct ypabact_data *ybd, struct assesstarget_msg *at );
struct Bacterium *yb_SearchBact( Object *world, ULONG commandid, UBYTE owner );


_dispatcher(void, yb_YBM_SETTARGET, struct settarget_msg *msg)
/*
**  FUNCTION
**
**      Setzt das sekundäre oder primäre Ziel. Dabei kann das Ziel als
**      Pointer oder Position übergeben werden. Ich belasse die settarget_msg
**      so, daß beides möglich ist. Es sei aber so, daß Bacterien als
**      Pointer und Sektoren als absolute x-z-Koordinaten übergeben werden.
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      13-Feb-96    floh+AF    + potentieller Bug elimiert, Tote
**                                akzeptieren keine Ziele mehr  
**                              + zweiter potentieller Bug elimiert.
**                                Mal sehen...
**      21-Mai-96   af Wenn wir ein Hauptziel setzen, welches kein Bakterium
**                  ist, dann löschen wir die PrimCommandID, bei Bakterien fragen wir
**                  ja sowieso nochmal nach, ob zu dem geschwader jemand existiert
**                  und bei NONE ignorieren wir das ganze, weil danach ja noch einer
**                  zu eben diesen Geschwader angemeldet werden kann.
**
**      2-Sep-96    Zielseetzung muß in jedem Falle möglich sein ( :-(( ), deshalb
**                  muß ich dort, wo ich das ziel setze, testen, ob ich das
**                  darf, der Empfänger schon tot ist oder so...
**
**      25-Okt-97   jaja, wir arbeiten immer noch dran und jetzt werden funda-
**                  mentale Sachen geändert: Es gibt kein Formationsziel mehr.
**                  Hauptziele werden rekursiv an alle Slaves vergeben, weil
**                  das Wegpunkte "benutzbar" macht und verhindert, daß ein
**                  Geschwader stehen bleibt, weil der Chef ein problem hat.
**
**      25-Okt-97   Eigentlich werden alle HauptZiele an die Slaves weitergegeben
**                  In einigen wenigen Fällen ist dies jedoch unerwünscht (Waypoint)
**                  Um nicht ein neues Feld einzuführen, mache ich ein neues Ziel,
**                  welches intern genauso wie sein Original ist!
**
*/
{
    struct ypabact_data *ybd;
    struct getsectorinfo_msg gsi;
    FLOAT  rand = SECTOR_SIZE + 10;

    ybd = INST_DATA( cl, o );

    /*** auf jeden Fall Test-Zeit rücksetzen!! ***/
    ybd->bact.assess_time = 0;
    
    if( (ybd->bact.ExtraState & EXTRA_LOGICDEATH) && (TARTYPE_BACTERIUM == msg->target_type) ) {
        _LogMsg("ALARM!!! bact-settarget auf logische Leiche owner %d, class %d, prio %d\n",
            ybd->bact.Owner, ybd->bact.BactClassID, msg->priority );
        return;
        }

    /*** Um welches Ziel handelt es sich? ***/
    if( msg->priority == 0 ) {

        /* ------------------------------------------
        **          Primary Target
        ** ----------------------------------------*/
        struct OBNode *slave;

        /* ----------------------------------------------------------
        ** Immer, wenn ich ein Ziel setze, muß ich testen, ob bisher
        ** das Ziel TARTYPE_BACTERIUM war, denn dann muß ich das alte
        ** Ziel rauslösen
        ** --------------------------------------------------------*/

        if( ybd->bact.PrimTargetType == TARTYPE_BACTERIUM ) {

            /*** Ich muß mich als Attacker abmelden ***/
            _Remove( (struct Node *) &(ybd->pt_attck_node) );
            
            /*** CommanderID darf hier nicht gelöscht werden !! ***/
            }
        
        switch( msg->target_type )  {


            case TARTYPE_SECTOR:
            case TARTYPE_SECTOR_NR:
                
                /* ----------------------------------------------------------
                ** Wir übergeben die Koordinaten dem WO und tragen alle
                ** Informationen, die das Wo uns über den Sektor gegeben hat,
                ** in die bact-Struktur ein. Sind die Koordinaten falsch,
                ** korrigiere ich sie in die Welt.
                ** --------------------------------------------------------*/

                ybd->bact.PrimCommandID  = 0;
                ybd->bact.PrimTargetType = TARTYPE_SECTOR;
                gsi.abspos_x = msg->pos.x;
                gsi.abspos_z = msg->pos.z;

                if( gsi.abspos_x <  rand )
                    gsi.abspos_x =  rand;
                if( gsi.abspos_x >  ybd->bact.WorldX - rand )
                    gsi.abspos_x =  ybd->bact.WorldX - rand;
                if( gsi.abspos_z > -rand )
                    gsi.abspos_z = -rand;
                if( gsi.abspos_z <  ybd->bact.WorldZ + rand )
                    gsi.abspos_z =  ybd->bact.WorldZ + rand;
                
                if( !_methoda( ybd->world, YWM_GETSECTORINFO, &gsi )) {

                    /*** Error ***/
                    ybd->bact.PrimaryTarget.Sector = NULL;
                    ybd->bact.PrimTargetType = TARTYPE_NONE;

                    }
                else {

                    /*** war alles ok. ***/
                    ybd->bact.PrimaryTarget.Sector = gsi.sector;

                    /*** Die Zielposition ***/
                    ybd->bact.PrimPos.x =  gsi.abspos_x;
                    ybd->bact.PrimPos.z =  gsi.abspos_z;

                    /*** sinnvoll ? Ja! ***/
                    ybd->bact.PrimPos.y = gsi.sector->Height;
                    }
                
                break;

            case TARTYPE_BACTERIUM:
            case TARTYPE_BACTERIUM_NR:

                /* ----------------------------------------------------------
                ** bei Bakterien sind wir immer in den Attck_lists der Opfer.
                ** Wir müssen uns also bei Bedarf ausklinken und wieder neu
                ** einklinken
                ** --------------------------------------------------------*/
                
                ybd->bact.PrimaryTarget.Bact =
                          (struct Bacterium *) msg->target.bact;
                ybd->bact.PrimTargetType = TARTYPE_BACTERIUM;

                /*** Die Position ist überflüssig...Aber egal... ***/
                if( ybd->bact.PrimaryTarget.Bact ) {

                    struct List *list;
                    
                    /*** Das Ziel schon tot? ***/
                    if( ybd->bact.PrimaryTarget.Bact->ExtraState & EXTRA_LOGICDEATH ) {
                        _LogMsg("totes vehicle als hauptziel, owner %d, class %d - ich bin class %d\n",
                                msg->target.bact->Owner, msg->target.bact->BactClassID, ybd->bact.BactClassID );
                        ybd->bact.PrimTargetType = TARTYPE_NONE;
                        return;
                        }
                        
                    ybd->bact.PrimPos.x = ybd->bact.PrimaryTarget.Bact->pos.x;
                    ybd->bact.PrimPos.y = ybd->bact.PrimaryTarget.Bact->pos.y;
                    ybd->bact.PrimPos.z = ybd->bact.PrimaryTarget.Bact->pos.z;

                    _get( ybd->bact.PrimaryTarget.Bact->BactObject, 
                          YBA_AttckList, &list);

                    if( list ) _AddTail( list,(struct Node *)&(ybd->pt_attck_node));
                    
                    /*** CommanderID eintragen, wenn er eine hat ***/
                    ybd->bact.PrimCommandID = msg->target.bact->CommandID;
                    }
                else {
                    
                    _LogMsg("PrimT. without a pointer\n");
                    ybd->bact.PrimTargetType = TARTYPE_NONE;
                    }


                break;

            case TARTYPE_FORMATION:

                ybd->bact.PrimCommandID  = 0;
                ybd->bact.PrimTargetType = TARTYPE_FORMATION;
                ybd->bact.PrimPos = msg->pos;
                break;

            case TARTYPE_NONE:

                /* ----------------------------------------------------
                ** Gibt es eigentlich nicht, sollte aber wegen diverser
                ** tests bleiben. Evtl. Todesfall rausfiltern. Um aus
                ** Nicht-Zielen Sektor-Ziele zu machen, besser extern
                ** setzen
                ** --------------------------------------------------*/
                ybd->bact.PrimTargetType = TARTYPE_NONE;
                ybd->bact.PrimaryTarget.Bact = NULL;
                ybd->bact.ExtraState   &= ~EXTRA_DOINGWAYPOINT;
                ybd->bact.num_waypoints = 0;
                break;

            case TARTYPE_SIMPLE:

                /* ------------------------------------------------------------
                ** Lohnt sich eigentlich nur für Raketen. Es wird in target.pos
                ** eine Richtung übergeben, die auf 1 normiert ist und die wir
                ** in tar_unit übernehmen. Dann fliegt das Ding immer in die
                ** Richtung
                ** Hat nur als Hauptziel Sinn
                ** ----------------------------------------------------------*/
                ybd->bact.tar_unit.x = msg->pos.x;
                ybd->bact.tar_unit.y = msg->pos.y;
                ybd->bact.tar_unit.z = msg->pos.z;
                ybd->bact.PrimTargetType     = TARTYPE_SIMPLE;
                ybd->bact.PrimaryTarget.Bact = NULL;
                ybd->bact.PrimCommandID      = 0;
                break;

            default:

                /* ------------------------------------------------------
                ** auf jeden Fall den Zieltyp setzen, damit ich weiß, daß
                ** es zumindest kein Bakterium war
                ** ----------------------------------------------------*/
                ybd->bact.PrimTargetType = msg->target_type;
                break;
            }

        /* ---------------------------------------------------------------
        ** Und nun an alle Slaves, falls nonrekursiv, NUR Zielanmeldungen,
        ** Abmeldung macht jeder sowieso für sich
        ** -------------------------------------------------------------*/
        if( (TARTYPE_SECTOR    == msg->target_type) ||
            (TARTYPE_BACTERIUM == msg->target_type) )
            for( slave = (struct OBNode *) ybd->bact.slave_list.mlh_Head;
                 slave->nd.mln_Succ;
                 slave = (struct OBNode *) slave->nd.mln_Succ ) {

                /*** keine leichen!!!! ***/
                if( ACTION_DEAD == slave->bact->MainState )
                    continue;

                /*** Ziel zuweisen ***/
                _methoda( slave->o, YBM_SETTARGET, msg );

                /*** Wenn ich nicht WAYPOINT, dann Slaves auch nicht! ***/
                if( !(ybd->bact.ExtraState & EXTRA_DOINGWAYPOINT))
                    slave->bact->ExtraState &= ~(EXTRA_DOINGWAYPOINT|
                                                 EXTRA_WAYPOINTCYCLE);
                }
        
        }
    else {

        /* --------------------------------------------
        **           Secondary Target
        ** ------------------------------------------*/

        /* ----------------------------------------------------------
        ** Immer, wenn ich ein Ziel setze, muß ich testen, ob bisher
        ** das Ziel TARTYPE_BACTERIUM war, denn dann muß ich das alte
        ** Ziel rauslösen
        ** --------------------------------------------------------*/

        if( ybd->bact.SecTargetType == TARTYPE_BACTERIUM ) {

            /*** Ich muß mich als Attacker abmelden ***/
            _Remove( (struct Node *) &(ybd->st_attck_node) );
            }


        switch( msg->target_type )  {

            case TARTYPE_SECTOR:
            case TARTYPE_SECTOR_NR:
                
                /* ------------------------------------------------------------
                ** Wir übergeben die Koordinaten dem WO und tragen alle
                ** Informationen, die das Wo uns über den Sektor gegeben hat,
                ** in die bact-Struktur ein. Ist der Sektor ungültig, so
                ** löschen wir das Ziel. 
                ** Es ist unwahrscheinlich, daß ein Sektor ein NZ ist, aber was 
                ** soll's...
                ** ----------------------------------------------------------*/

                ybd->bact.SecTargetType = TARTYPE_SECTOR;
                gsi.abspos_x = msg->pos.x;
                gsi.abspos_z = msg->pos.z;

                if( gsi.abspos_x <  rand )
                    gsi.abspos_x =  rand;
                if( gsi.abspos_x >  ybd->bact.WorldX - rand )
                    gsi.abspos_x =  ybd->bact.WorldX - rand;
                if( gsi.abspos_z > -rand )
                    gsi.abspos_z = -rand;
                if( gsi.abspos_z <  ybd->bact.WorldZ + rand )
                    gsi.abspos_z =  ybd->bact.WorldZ + rand;
                
                if( !_methoda( ybd->world, YWM_GETSECTORINFO, &gsi )) {

                    /*** Error ***/
                    ybd->bact.SecondaryTarget.Sector = NULL;
                    ybd->bact.SecTargetType = TARTYPE_NONE;

                    }
                else {

                    /*** war alles ok. ***/
                    ybd->bact.SecondaryTarget.Sector = gsi.sector;
                    ybd->bact.SecPos.x = gsi.abspos_x;
                    ybd->bact.SecPos.z = gsi.abspos_z;

                    /*** sinnvoll ? Ja! ***/
                    ybd->bact.SecPos.y = gsi.sector->Height;
                    }


                break;

            case TARTYPE_BACTERIUM:
            case TARTYPE_BACTERIUM_NR:

                /* ----------------------------------------------------------
                ** bei Bakterien sind wir immer in den Attck_lists der Opfer.
                ** Wir müssen uns also bei Bedarf ausklinken und wieder neu
                ** einklinken
                ** --------------------------------------------------------*/
                ybd->bact.SecondaryTarget.Bact =
                          (struct Bacterium *) msg->target.bact;
                ybd->bact.SecTargetType = TARTYPE_BACTERIUM;

                /*** Die Position ist überflüssig...Aber egal... ***/

                if( ybd->bact.SecondaryTarget.Bact ) {

                    struct List *list;

                    /*** Das Ziel schon tot? ***/
                    if( ybd->bact.SecondaryTarget.Bact->ExtraState & EXTRA_LOGICDEATH ) {
                        _LogMsg("totes vehicle als nebenziel, owner %d, class %d\n",
                                msg->target.bact->Owner, msg->target.bact->BactClassID );
                        ybd->bact.SecTargetType = TARTYPE_NONE;
                        return;
                        }
                        
                    ybd->bact.SecPos.x = ybd->bact.SecondaryTarget.Bact->pos.x;
                    ybd->bact.SecPos.y = ybd->bact.SecondaryTarget.Bact->pos.y;
                    ybd->bact.SecPos.z = ybd->bact.SecondaryTarget.Bact->pos.z;

                    _get( ybd->bact.SecondaryTarget.Bact->BactObject,
                          YBA_AttckList, &list);

                    if( list ) 
                        _AddTail( list,(struct Node *)&(ybd->st_attck_node));
                    else
                        _LogMsg("Net eigeklink\n");
                    }
                else {
                    _LogMsg("Yppsn\n");
                    ybd->bact.SecTargetType = TARTYPE_NONE;
                    }

                break;

            case TARTYPE_FORMATION:

                /* ------------------------------------------------------
                ** wird für Sekundärziele eigentlich nicht unterstützt...
                ** nur der Form halber
                ** ----------------------------------------------------*/
                ybd->bact.SecTargetType = TARTYPE_FORMATION;
                ybd->bact.SecPos = msg->pos;
                break;

            case TARTYPE_NONE:

                ybd->bact.SecTargetType = TARTYPE_NONE;
                ybd->bact.SecondaryTarget.Bact = NULL;
                break;

            default:

                /* ------------------------------------------------------
                ** auf jeden Fall den Zieltyp setzen, damit ich weiß, daß
                ** es zumindest kein Bakterium war
                ** ----------------------------------------------------*/
                ybd->bact.SecTargetType = msg->target_type;
                break;
            }

        }
}


_dispatcher( ULONG, yb_YBM_ASSESSTARGET, struct assesstarget_msg *at )
{
/*
**  FUNCTION    Das problem ist folgendes: Ob ich ein Ziel bearbeite (also
**              hingehe oder bekämpfe), hängt von meiner Verfassung (Aggr),
**              der Entfernung (auch Sichtbarkeit) und den eigentümern
**              ab. Weil dazu an mehreren stellen Entscheidungen getroffen
**              werden müssen (fight, ai3-wait, ...) und die Code-Bestandteile
**              z.T. recht unterschiedlich sind, muß es mal eine Institution
**              geben, die Entscheidet, ob wir das Ziel
**                  ignorieren
**                  dorthin gehen
**                  bekämpfen
**                  abmelden
**              Das macht diese Routine, so daß es möglichst nicht mehr vorkommt, 
**              daß in Wait etwas als Ziel akzeptiert wird, welches fight dann
**              wieder ablehnt, so daß es zu "Umschalt-Rucklern" kommt.
**
**              Wir geben hier außerdem eine Maske an, welche Zielarten behandelt
**              werden sollen, also TARTYPE_S/B/F. Dann wird zuerst das Neben-
**              (sofern sinnvoll), dann das Hauptziel untersucht.
**
**              Ein fremdes Ziel kann ich bekämpfen oder abmelden, ein eigenes
**              ignorieren (wenn ich schon dort bin) oder eben hinfahren.
**              Die Zielabmeldung sollte 0 sein (HT_REMOVE)
**
**  RETURN      HT_REMOVE, HT_GOTO, HT_FIGHT, HT_IGNORE
**
**  CHANGED     8100 000C, der vom eigenen Code langsam die Schnauze voll hat...
**              18-Mai-96 Sektor 0 ist nun feindlich
**
**              Wegpunktsonderbehandlung, wenn es sich um Sektorhauptziele
**              handelt.
*/

    UBYTE  footprint;
    FLOAT  distance, secpos_x, secpos_z;
    struct Cell *sector;
    struct Bacterium *enemy;
    WORD   aggr;
    BOOL   sec_target, near_prim_target = FALSE, prim_done = FALSE;
    LONG   sectorenergy;
    struct ypabact_data *ybd = INST_DATA( cl, o);

    footprint = (1 << ybd->bact.Owner);


    if( at->target_type == TARTYPE_FORMATION ) {

        /*  -------------------------------------------------------------------
        **  Formationsziele sollten wir immer bearbeiten, denn dann entscheidet
        **  unser Chef darüber. Also müssen wir gucken, ob ein solches gesetzt
        **  wurde.
        **  -----------------------------------------------------------------*/

        if( (ybd->bact.PrimTargetType == TARTYPE_FORMATION) ||
            (ybd->bact.SecTargetType  == TARTYPE_FORMATION) )
            return( AT_GOTO );

        /*** andernfalls eben weitergucken ***/
        }


    if( at->target_type == TARTYPE_NONE ) {

        /*** Immer ignore, damit Wait nichts macht ***/
        return( AT_IGNORE );
        } 
        
    /* -----------------------------------------------
    ** Zielnaehe feststellen. Zielnaehe ist hoechstens
    ** Nachbarsektor!
    ** ---------------------------------------------*/
    near_prim_target = FALSE;
    if( TARTYPE_SECTOR == ybd->bact.PrimTargetType ) {
    
        if( nc_sqrt( (ybd->bact.pos.x - ybd->bact.PrimPos.x) *
                     (ybd->bact.pos.x - ybd->bact.PrimPos.x) +
                     (ybd->bact.pos.z - ybd->bact.PrimPos.z) *
                     (ybd->bact.pos.z - ybd->bact.PrimPos.z)) <
                     (1.5 * SECTOR_SIZE) )
            near_prim_target = TRUE;
        if( ybd->bact.Owner == ybd->bact.PrimaryTarget.Sector->Owner )
            prim_done = TRUE;             
        }   

    if( TARTYPE_BACTERIUM == ybd->bact.PrimTargetType ) {
    
        if( nc_sqrt( (ybd->bact.pos.x - ybd->bact.PrimaryTarget.Bact->pos.x) *
                     (ybd->bact.pos.x - ybd->bact.PrimaryTarget.Bact->pos.x) +
                     (ybd->bact.pos.z - ybd->bact.PrimaryTarget.Bact->pos.z) *
                     (ybd->bact.pos.z - ybd->bact.PrimaryTarget.Bact->pos.z)) <
                     (1.5 * SECTOR_SIZE) )
            near_prim_target = TRUE;
        if( ybd->bact.Owner == ybd->bact.PrimaryTarget.Bact->Owner )
            prim_done = TRUE;                 
        }   


    if( at->target_type == TARTYPE_BACTERIUM ) {

        /*** Welches Ziel ist es denn ? ***/
        enemy = NULL;
        if( at->priority == 1 ) {
            sec_target = TRUE;
            enemy      = ybd->bact.SecondaryTarget.Bact;
            aggr       = AGGR_FIGHTSECBACT;
            }
        else
            if( at->priority == 0 ) {
                sec_target = FALSE;
                enemy      = ybd->bact.PrimaryTarget.Bact;
                aggr       = AGGR_FIGHTPRIM;
                }

        /*** Nun mal genauer ansehen ***/
        if( enemy ) {

            /*** erstmal Entfernug ermitteln ***/
            distance = nc_sqrt( (enemy->pos.x - ybd->bact.pos.x) * 
                                (enemy->pos.x - ybd->bact.pos.x) +
                                (enemy->pos.z - ybd->bact.pos.z) *
                                (enemy->pos.z - ybd->bact.pos.z) );

            /*** Ist das sichtbar? ***/
            if( enemy->Sector->FootPrint & footprint ) {

                /*** Immer-Kampf-Aggression? ***/
                if( ybd->bact.Aggression >= AGGR_ALL ) {

                    /* ------------------------------------------------------
                    ** Bekämpfen, egal, was. Nebenziele aber auch bei grosser
                    ** entfernung abmelden. 
                    ** ----------------------------------------------------*/
                    if( sec_target ) {
                        
                        if( distance > SECTARGET_FORGET )
                            return( AT_REMOVE );
                        else
                            return( AT_FIGHT );
                        }
                    else
                        return( AT_FIGHT );
                    }
                else {

                    /*** Wem gehört es? ***/
                    if( (enemy->Owner == 0) || (enemy->Owner == ybd->bact.Owner) ) {

                        /*** freundlich. in der Nähe? ***/
                        if( distance < TAR_DISTANCE )
                            return( AT_IGNORE );
                        else
                            return( AT_GOTO );
                        }
                    else {

                        /*** feindlich. auf der Flucht? ***/
                        if( ybd->bact.ExtraState & EXTRA_ESCAPE ) {

                            /* --------------------------------------------------------
                            ** Uns gehts so schlecht, daß wir normalerweise nix machen,
                            ** aber am Robo (am hauptziel wenn Flucht) kaempfen wir 
                            ** ------------------------------------------------------*/
                            if( near_prim_target )
                                return( AT_FIGHT );
                            else    
                                return( AT_REMOVE );
                            }
                        else {

                            /*** reicht unsere Aggression für dieses Ziel? ***/
                            if( ybd->bact.Aggression >= aggr ) {

                                /* -----------------------------------------
                                ** Bei Feinden gibt es kein Goto. Entfernung
                                ** entscheidet CAFP 
                                ** ---------------------------------------*/
                                if( sec_target ) {

                                    /* ----------------------------------------
                                    ** Sonderbehandlung Nebenziele: Wenn ich
                                    ** weit weg vom HZ bin, teste ich, wieviele
                                    ** schon daran arbeiten. Sind es mehr als
                                    ** 2, gehe ich freiwillig.
                                    **
                                    ** Noch was: Wenn ich zu weit weg vom neben-
                                    ** ziel bin, schalte ich es ab. 
                                    ** --------------------------------------*/
                                    if( BCLID_YPAGUN != ybd->bact.BactClassID) {

                                        FLOAT  dist;
                                        struct flt_triple tpos;
                                        
                                        /*** Zu weit weg? ***/
                                        if( distance > SECTARGET_FORGET )
                                            return( AT_REMOVE );

                                        // Das Hauptziel kennt meistens nur der Chef...
                                        if( ybd->bact.master == ybd->bact.robo ) {

                                            // I'm the Chief
                                            if( TARTYPE_SECTOR ==  ybd->bact.PrimTargetType)
                                                tpos = ybd->bact.PrimPos;
                                            else
                                                if( TARTYPE_BACTERIUM ==
                                                    ybd->bact.PrimTargetType)
                                                    tpos = ybd->bact.PrimaryTarget.Bact->pos;
                                                else
                                                    tpos = ybd->bact.pos;     // weil unbekannte Zielart
                                            }
                                        else {

                                            // Chef erst ermitteln
                                            struct Bacterium *chef;
                                            _get( ybd->bact.master, YBA_Bacterium, &chef );

                                            if( TARTYPE_SECTOR ==  chef->PrimTargetType)
                                                tpos = chef->PrimPos;
                                            else
                                                if( TARTYPE_BACTERIUM ==  chef->PrimTargetType)
                                                    tpos = chef->PrimaryTarget.Bact->pos;
                                                else
                                                    tpos = ybd->bact.pos;     // weil unbekannte Zielart
                                            }

                                        dist = nc_sqrt( (tpos.x-ybd->bact.pos.x)*
                                                        (tpos.x-ybd->bact.pos.x)+
                                                        (tpos.z-ybd->bact.pos.z)*
                                                        (tpos.z-ybd->bact.pos.z));

                                        // Test nur, wenn wir zuweit weg vom HZ sind
                                        if( dist > FAR_FROM_SEC_TARGET ) {

                                            struct MinList *alist;
                                            struct OBNode  *anode;
                                            ULONG  count = 0;
                                            
                                            /* ---------------------------------------------
                                            ** achtung!!! im gegensatz zur Zielaufnahme muss
                                            ** ich hier anstatt mit der 1, mit der 2 testen, 
                                            ** denn ich zaehle mich ja mit!!!!!!!!!!!!!!
                                            ** -------------------------------------------*/
                                            _get( ybd->bact.SecondaryTarget.Bact->BactObject,
                                                  YBA_AttckList, &alist );
                                            anode = (struct OBNode *) alist->mlh_Head;
                                            while( anode->nd.mln_Succ ) {

                                                struct OBNode *st_node;
                                                _get( anode->o, YBA_StAttckNode, &st_node );
                                                if( (anode == st_node) &&
                                                    (anode->bact->Owner == ybd->bact.Owner) )
                                                    count++;
                                                if( count > 2) break; // 2 leute...
                                                anode = (struct OBNode *) anode->nd.mln_Succ;
                                                }

                                            /*** Arbeiten schon zuviele leute dran? ***/
                                            if( count > 2 ) {

                                                // nein danke
                                                return( AT_REMOVE );
                                                }
                                            }
                                        }
                                    return( AT_FIGHT );
                                    }
                                else
                                    return( AT_FIGHT );
                                }
                            else {

                                /* ------------------------------------------------
                                ** Nix für uns. Es sei denn, wir sind am Hauptziel,
                                ** dann konnten wir voellig legal was aufnehmen. 
                                ** ----------------------------------------------*/
                                if( near_prim_target && prim_done )
                                    return( AT_FIGHT );
                                else
                                    return( AT_REMOVE );
                                }
                            }
                        }
                    }
                }
            else {

                /*** Nicht sichtbar, auf jeden Fall abmelden ***/
                return( AT_REMOVE );
                }
            }
        }


    if( at->target_type == TARTYPE_SECTOR ) {

        /*** Welches Ziel ist es denn ? ***/
        sector = NULL;
        if( ybd->bact.SecTargetType == TARTYPE_SECTOR ) {
            sec_target = TRUE;
            sector     = ybd->bact.SecondaryTarget.Sector;
            secpos_x   = ybd->bact.SecPos.x;
            secpos_z   = ybd->bact.SecPos.z;
            aggr       = AGGR_FIGHTSECSEC;
            }
        else
            if( ybd->bact.PrimTargetType == TARTYPE_SECTOR ) {
                sec_target = FALSE;
                sector     = ybd->bact.PrimaryTarget.Sector;
                secpos_x   = ybd->bact.PrimPos.x;
                secpos_z   = ybd->bact.PrimPos.z;
                aggr       = AGGR_FIGHTPRIM;
                }
            
        /*** Wegpunkt-Sonderbehandlung bei Hauptzielen ***/
        if( (ybd->bact.ExtraState & EXTRA_DOINGWAYPOINT) &&
            (FALSE == sec_target) )
            return( yb_DoingWaypoint( ybd, at ) );

        /*** Nun mal genauer ansehen ***/
        if( sector ) {

            /*** Mal Energie ermitteln ***/
            if( sector->SType == SECTYPE_COMPACT )
                sectorenergy = sector->SubEnergy[0][0];
            else {

                int iks, yps;
                sectorenergy = 0;

                for( iks = 0; iks < 3; iks++ )
                    for( yps = 0; yps < 3; yps++ )
                        sectorenergy += sector->SubEnergy[ iks ][ yps ];
                }
        
            /*** Entfernung ***/
            distance = nc_sqrt( (ybd->bact.pos.x - secpos_x) * 
                                (ybd->bact.pos.x - secpos_x) +
                                (ybd->bact.pos.z - secpos_z) *
                                (ybd->bact.pos.z - secpos_z) );
            
            /*** Also, los geht es. Volle aggression? ****/
            if( ybd->bact.Aggression >= AGGR_ALL ) {

                /*** nicht Kämpfen, wenn schon da und Sektor auf 0 ***/
                if( (sectorenergy <= 0) && (sector->Owner == ybd->bact.Owner) ) {
           
                    /* ----------------------------------------------
                    ** eigentlich abmelden, aber Tanks erreichen beim
                    ** Kampf den Punkt nicht. Deshalb hinfahren.
                    ** --------------------------------------------*/
                    if( distance < TAR_DISTANCE ) 
                        return( AT_IGNORE );   // return( AT_REMOVE );
                    else
                        return( AT_GOTO );
                    }
                else
                    return( AT_FIGHT );
                }
            else {

                /*** sind wir schon da? ***/
                if( distance < TAR_DISTANCE ) {

                    /*** ich bin all hier ***/
                    if( sector->Owner == ybd->bact.Owner ) {

                        /* -------------------------------------------------
                        ** Was eigenes. Wenn es ein Nebenziel ist, abmelden.
                        ** Sonst nüscht machen
                        ** -----------------------------------------------*/
                        if( sec_target )
                            return( AT_REMOVE );
                        else
                            return( AT_IGNORE );
                        }
                    else {

                        /*** Feind. Flüchten wir? ***/
                        if( ybd->bact.ExtraState & EXTRA_ESCAPE ) {

                            /*** Wieder abmelden ***/
                            return( AT_REMOVE );
                            }
                        else {

                            /*** reicht die Aggression für einen Kampf? ***/
                            if( ybd->bact.Aggression >= aggr ) {

                                /*** fein. lets baller! ***/
                                return( AT_FIGHT );
                                }
                            else {

                                /*** Tja, dann eben abmelden ***/
                                return( AT_REMOVE );
                                }
                            }
                        }
                    }
                else {

                    /*** weit weg. Wem gehört'n das? ***/
                    if( sector->Owner == ybd->bact.Owner ) {

                        /* ----------------------------------------
                        ** freundlich gesinnt. nüscht wie hin, oder
                        ** abmelden, wenn es ein Nebenziel war.
                        ** --------------------------------------*/
                        if( sec_target )
                            return( AT_REMOVE );
                        else
                            return( AT_GOTO );
                        }
                    else {

                        /*** böse. Hauen wir gerade ab? ***/
                        if( ybd->bact.ExtraState & EXTRA_ESCAPE ) {

                            /*** abmelden ***/
                            return( AT_REMOVE );
                            }
                        else {

                            /*** reicht Aggression aus ? ***/
                            if( ybd->bact.Aggression >= aggr ) {

                                /*** kämpfen hat nix mit Entfernung zu tun ***/
                                return( AT_FIGHT );    // weil wir erst hinmüssen!
                                }
                            else {

                                /*** Abmelden ***/
                                return( AT_REMOVE );
                                }
                            }
                        }
                    }
                }
            }
        }

    /* -----------------------------------------------------------
    ** Wenn wir bis hierher kamen, dann haben wir nichts machbares 
    ** gefunden. Kann sein, daß wir z.B. schon da sind oder so 
    ** ---------------------------------------------------------*/
    return( AT_IGNORE );
}


_dispatcher( BOOL, yb_YBM_TESTSECTARGET, struct Bacterium *enemy )
{
/*
**  FUNCTION    Testet ein Nebenziel, ob es günstig ist. Diese
**              Methode macht hier, na, wißt ihr es schon? Genau.
**              Nichts!
**              Denn erst bei kuriosen Sachen wie den Flaks lohnt 
**              sich sowas und dort überlagern wir es.
**
**              Dient auch als test, ein Ziel wieder abzumelden
**              (also kann dafür verwendet werden...)
**
**  INPUT       Bacterium des feindes
**
**  RESULT      TRUE, wenn ok
**
**  CHANGED     created Ei Polterabend
*/

    return( TRUE );
}


ULONG yb_DoingWaypoint( struct ypabact_data *ybd, struct assesstarget_msg *at )
{
    /*
    ** Erledigt die Sonderbehandlung für Wegpunkte. Im Gegensatz zum normalen
    ** Test werden die Ziele evtl. nicht angegriffen, obwohl sie feindlich
    ** sind, sondern weitergeschalten.
    ** Alle Waypoints müssen als Positionen und nicht als Vehicle übergeben
    ** werden!
    **
    ** Wir testen, ob wir in der Nähe des Zielpunktes sind. Ist dem so,
    ** schalten wir je nach Modus den nächsten Punkt ein. In jedem Falle
    ** geben wir AT_GOTO zurück. Ausnahme ist, wenn wir im "Nicht-Cycle"-
    ** Modus sind und den letzten Zielpunkt ***eingeschalten*** haben.
    ** Dann schalten wir den Cycle-Modus einfach aus.
    ** Ziel schalten heißt: SETTARGET Sektorhauptziel.
    **
    ** SETTARGET hier NONREKURSIV! Ein neues Ziel ist einfacher als
    ** ein neues feld, wo dann auch andere Leute ändern müssen.
    */

    FLOAT distance;

    distance = nc_sqrt( (ybd->bact.pos.x - ybd->bact.PrimPos.x) *
                        (ybd->bact.pos.x - ybd->bact.PrimPos.x) +
                        (ybd->bact.pos.z - ybd->bact.PrimPos.z) *
                        (ybd->bact.pos.z - ybd->bact.PrimPos.z) );

    /*** Sind wir in Zielnähe? ***/
    if( distance < TAR_DISTANCE ) {

        int    i;
        struct settarget_msg target;

        /*** Also neu schalten. Welcher Modus? ***/
        if( ybd->bact.ExtraState & EXTRA_WAYPOINTCYCLE ) {

            /* ---------------------------------------------------------
            ** Haben wir den letzten Punkt erreicht, dann geht es wieder
            ** am Anfang los
            ** -------------------------------------------------------*/
            i = ++ybd->bact.count_waypoints;

            if( ybd->bact.count_waypoints >= ybd->bact.num_waypoints) {
                ybd->bact.count_waypoints = 0;
                i = 0;
                }

            /*** Ziel setzen ***/
            target.target_type = TARTYPE_SECTOR_NR;
            target.priority    = 0;
            target.pos.x       = ybd->bact.waypoint[ i ].x;
            target.pos.z       = ybd->bact.waypoint[ i ].z;
            _methoda( ybd->bact.BactObject, YBM_SETTARGET, &target );
            }
        else {

            /*** einmalige Fahrt. nächsten Zielpunkt kopieren ***/
            i = ++ybd->bact.count_waypoints;

            /*** Ziel setzen ***/
            if( ybd->bact.num_waypoints > 1 ) {

                target.target_type = TARTYPE_SECTOR_NR;
                target.priority    = 0;
                target.pos.x       = ybd->bact.waypoint[ i ].x;
                target.pos.z       = ybd->bact.waypoint[ i ].z;
                _methoda( ybd->bact.BactObject, YBM_SETTARGET, &target );
                }

            /*** Hochzählen und Abbruch ***/
            if( ybd->bact.count_waypoints >= (ybd->bact.num_waypoints-1) ) {

                /* ----------------------------------------------------
                ** Es war der letzte. Das kann aber auch ein Bacterium
                ** gewesen sein. Wenn ja, Ziel ersetzen. Weil ich
                ** (in diesem Falle) Commander bin, auch an alle Unter-
                ** gebenen.
                ** --------------------------------------------------*/
                if( ybd->bact.mt_commandid ) {

                    struct Bacterium *t;

                    /*** Ziel suchen ***/
                    if( t = yb_SearchBact( ybd->world, ybd->bact.mt_commandid,
                                           ybd->bact.mt_owner ) ) {
                                           
                        /*** noch sichtbar? ***/
                        if( t->Sector->FootPrint & (UBYTE)(1<<ybd->bact.Owner) ) {

                            target.target_type = TARTYPE_BACTERIUM;
                            target.priority    = 0;
                            target.target.bact = t;
                            _methoda( ybd->bact.BactObject, YBM_SETTARGET, &target );
                            }
                        }
                    }

                ybd->bact.mt_commandid = 0L;
                ybd->bact.mt_owner     = 0;

                ybd->bact.ExtraState &= ~(EXTRA_DOINGWAYPOINT|
                                          EXTRA_WAYPOINTCYCLE);
                }
            }
        }

    return( AT_GOTO );
}

void yb_CopyWayPoints( struct Bacterium *from, struct Bacterium *to )
{
    /* ------------------------------------------------------------------
    ** Kopiert Wegpunkte (wenn notwendig). Das Löschen bei der
    ** Quelle könnte hier mit geschehen, erscheint mir derzeit aber nicht
    ** notwendig
    ** ----------------------------------------------------------------*/
    if( from->ExtraState & EXTRA_DOINGWAYPOINT ) {

        int i;

        /*** Punkte ***/
        for( i = 0; i < MAXNUM_WAYPOINTS; i++ )
            to->waypoint[ i ] = from->waypoint[ i ];

        /*** Flags ***/
        to->ExtraState |= EXTRA_DOINGWAYPOINT;
        if( from->ExtraState & EXTRA_WAYPOINTCYCLE )
            to->ExtraState |=  EXTRA_WAYPOINTCYCLE;
        else
            to->ExtraState &= ~EXTRA_WAYPOINTCYCLE;

        /*** Zähler ***/
        to->num_waypoints   = from->num_waypoints;
        to->count_waypoints = from->count_waypoints;
        }
}


struct Bacterium *yb_SearchBact( Object *world, ULONG commandid, UBYTE owner )
{
    /* ---------------------------------------------------------------------------
    ** Sucht an Hand der übergebenen Informationen einen Commander. Da dieser fuer
    ** Zielsetzungen verwendet wird, sollte er nicht tot sein. 
    ** -------------------------------------------------------------------------*/
    struct ypaworld_data *ywd;
    struct OBNode *robo;

    ywd  = INST_DATA( ((struct nucleusdata *)world)->o_Class, world );
    robo = (struct OBNode *) ywd->CmdList.mlh_Head;

    while( robo->nd.mln_Succ ) {

        if( (BCLID_YPAROBO == robo->bact->BactClassID) &&
            (owner         == robo->bact->Owner) ) {

            struct OBNode *commander;

            /*** Ok, der Robo. wars der zufaellig? ***/
            if( robo->bact->CommandID == commandid ) {
                if( ACTION_DEAD == robo->bact->MainState )
                    return( NULL );
                else
                    return( robo->bact );
                }

            commander = (struct OBNode *) robo->bact->slave_list.mlh_Head;
            while( commander->nd.mln_Succ ) {

                if( commandid == commander->bact->CommandID ) {
                    if( ACTION_DEAD == commander->bact->MainState )
                        return( NULL );
                    else
                        return( commander->bact );
                    }
                    
                commander = (struct OBNode *) commander->nd.mln_Succ;
                }
            }

        robo = (struct OBNode *) robo->nd.mln_Succ;
        }

    return( NULL );
}
