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

#include "nucleus/nucleus2.h"
#include "engine/engine.h"
#include "transform/tform.h"
#include "ypa/ypaworldclass.h"
#include "ypa/ypabactclass.h"
#include "input/input.h"
#include "ypa/ypatankclass.h"

#ifdef _MSC_VER
#define min(a,b) ((a)<(b)?(a):(b))
#endif

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine

struct Cell *yb_GetSector( struct ypaworld_data *ywd, WORD pos_x, WORD pos_y);
FLOAT yb_g( WORD from_x, WORD from_y, WORD to_x, WORD to_y );
FLOAT yb_f( WORD from_x, WORD from_y, struct Cell *from_sector,
            WORD to_x, WORD to_y, struct Cell *to_sector );
void  yb_GiveWayPointToSlaves( struct Bacterium *bact, WORD count );

_dispatcher(BOOL, yb_YBM_SETWAY, struct findpath_msg *fpath)
{
        /*
        **  FUNCTION    kapselt das Zielsetzen fuer Panzer. Ziel
        **              ist, diese Routine an Stelle von SetTarget
        **              aufzurufen. Egal, ob Einzelziel oder
        **              Zwischenwaypoints, alles ist hier drin.
        **
        **  INPUT       Der Ausgangspunkt und der Endpunkt
        **              und die Zahl der Wegpunkte, die uns
        **              zur Verfuegung stehen. Evtl. noch Flags
        **              um AddWaypoint leichter identifizieren
        **              zu koennen.
        **
        **              Achtung, irgendwelche Umschichtungen der
        **              Geschwader erfolgt extern!
        **
        **  RESULT      TRUE, wenn es moeglich war, einen Weg zu fin-
        **              den.
        */
        BOOL   ret;
        struct settarget_msg target;
        struct ypabact_data *ybd;
        WORD   merke_waypoints;

        ybd = INST_DATA( cl, o );
        merke_waypoints = fpath->num_waypoints;

        switch( fpath->flags ) {

            case WPF_Normal:

                /*** Pfad ermitteln lassen ***/
                if( _methoda( o, YBM_FINDPATH, fpath ) ) {

                    struct OBNode *slave;

                    /*** ok, Ziel und evtl. Wegpunkte setzen ***/
                    if( fpath->num_waypoints > 1 ) {

                        int i;

                        for( i=0; i<fpath->num_waypoints;i++)
                            ybd->bact.waypoint[i] = fpath->waypoint[i];

                        ybd->bact.ExtraState     |= EXTRA_DOINGWAYPOINT;
                        ybd->bact.num_waypoints   = fpath->num_waypoints;
                        ybd->bact.count_waypoints = 0;

                        yb_GiveWayPointToSlaves( &(ybd->bact), 0 );

                        target.pos.x   = fpath->waypoint[0].x;
                        target.pos.z   = fpath->waypoint[0].z;
                        }
                    else {

                        /*** Direkt durch ***/
                        target.pos.x   = fpath->to_x;
                        target.pos.z   = fpath->to_z;
                        }

                    target.target_type = TARTYPE_SECTOR;
                    target.priority    = 0;
                    _methoda( o, YBM_SETTARGET, &target);

                    /* ----------------------------------------
                    ** Jetzt noch mal alle Slaves durchgehen,
                    ** die Panzer sind und nicht auf dem Sektor
                    ** des Commanders stehen. Die bekommen eine
                    ** eigene Wegliste.
                    ** --------------------------------------*/
                    slave = (struct OBNode *) ybd->bact.slave_list.mlh_Head;
                    while( slave->nd.mln_Succ ) {

                        if( ((BCLID_YPACAR   ==slave->bact->BactClassID) ||
                             (BCLID_YPATANK  ==slave->bact->BactClassID)) &&
                            (ybd->bact.Sector!=slave->bact->Sector) ) {

                            /*** rekursiver Aufruf ***/
                            struct findpath_msg fp2;

                            fp2.num_waypoints = merke_waypoints;
                            fp2.from_x        = slave->bact->pos.x;
                            fp2.from_z        = slave->bact->pos.z;
                            fp2.to_x          = fpath->to_x;
                            fp2.to_z          = fpath->to_z;
                            fp2.flags         = fpath->flags;
                            _methoda( slave->o, YBM_SETWAY, &fp2 );
                            }

                        slave = (struct OBNode *) slave->nd.mln_Succ;
                        }

                    /*** erfolgreich ***/
                    ret = TRUE;
                    }
                else
                    ret = FALSE;

                break;

            case WPF_StartWayPoint:

                break;

            case WPF_AddWayPoint:

                break;
            }

    return( ret );
}


_dispatcher(BOOL, yb_YBM_FINDPATH, struct findpath_msg *fpath)
{
    /*
    **  FUNCTION    sucht einen Weg von einem Sektor zum anderen.
    **              Dabei werden nur Sektoren beachtet. 
    **              Die Mittelpunkte der Teilsektoren werden als
    **              Wegpunktziele zurueckgegeben. Sofern man direkt
    **              durchfahren kann, geben wir nur den Zielsektor
    **              zurueck.
    **
    **  INPUT       Start und Ziel als Koordinaten.
    **
    **  RESULT      TRUE:
    **              Wegpunkte oder nur Zielsektor, der dann als Ziel
    **              direkt gesetzt werden kann. Ansonsten waren es
    **              eben Waypoint-ziele.
    **              FALSE:
    **              keinen Weg gefunden, num_waypoints = 0
    **
    **  CHANGED
    */

    struct Cell *sector, *to_sector, *standing_sector;
    struct Cell *next_sector, *running_sector;
    WORD   sec_from_x, sec_from_y, sec_to_x, sec_to_y;
    WORD   x, y, standing_x, standing_y;
    FLOAT  value;
    struct MinList way_list, open_list;
    WORD   dir_x, dir_y, wp;
    struct ypaworld_data *ywd;
    struct ypabact_data *ybd;
    WORD   num_waypoints;


    ybd = INST_DATA( cl, o );
    ywd = INST_DATA( ((struct nucleusdata *)ybd->world)->o_Class, ybd->world );
    num_waypoints = fpath->num_waypoints;

    /*** voher Aufraeumen! Flags und Listen ***/
    for( x = 0; x < ywd->MapSizeX; x++ ) {
        for( y = 0; y < ywd->MapSizeY; y++ ) {
            ywd->CellArea[ x + y * ywd->MapSizeX ].PathFlags         = 0;
            ywd->CellArea[ x + y * ywd->MapSizeX ].FPathValue        = 0;
            ywd->CellArea[ x + y * ywd->MapSizeX ].GPathValue        = 0;
            ywd->CellArea[ x + y * ywd->MapSizeX ].TreeNode.mln_Succ = NULL;
            ywd->CellArea[ x + y * ywd->MapSizeX ].TreeNode.mln_Pred = NULL;
            ywd->CellArea[ x + y * ywd->MapSizeX ].TreeUp            = NULL;
            }
        }

    /*** Sektoren ermitteln ***/
    sec_from_x = (WORD) ((fpath->from_x)  / SECTOR_SIZE);
    sec_from_y = (WORD) ((-fpath->from_z) / SECTOR_SIZE);
    sec_to_x   = (WORD) ((fpath->to_x)  / SECTOR_SIZE);
    sec_to_y   = (WORD) ((-fpath->to_z) / SECTOR_SIZE);
    to_sector  = yb_GetSector( ywd, sec_to_x, sec_to_y );

    /*** Falls Startsektor == ZielSektor sofort raus ***/
    if( (sec_from_x == sec_to_x) &&
        (sec_from_y == sec_to_y) ) {

        fpath->num_waypoints = 1;
        fpath->waypoint[0].x = fpath->to_x;
        fpath->waypoint[0].z = fpath->to_z;
        return( TRUE );
        }

    /*** Listen initialisieren ***/
    _NewList( (struct List *) &open_list );

    /*** Sektor, auf dem wir stehen, in close-list(!)... ***/
    sector = yb_GetSector( ywd, sec_from_x, sec_from_y );
    sector->PathFlags |= SPF_InClose;
    standing_x         = sec_from_x;
    standing_y         = sec_from_y;
    standing_sector    = sector;
    _NewList( (struct List *) &(standing_sector->TreeList) );

    /*** ...und werten ***/
    sector->FPathValue = 0.0;
    sector->GPathValue = yb_g( sec_from_x, sec_from_y, sec_to_x, sec_to_y );

    /* ---------------------------------------------------------------------
    ** Nun passiert folgendes. Jede Node in der open_list bekommt eine
    ** Wertung. Diese setzt sich aus 2 Komponenten zusammen: Der Wertig-
    ** keit bis dahin und der erarteten Wertigkeit bis zum Ziel. Die
    ** Wertigkeit zum Standpunkt wird als f, die zum Ziel als g bezeichnet.
    ** f muessen wir uns in der Sektorstruktur merken, g koennen wir
    ** jedesmal neu berechnen.
    **
    ** Wir suchen zum Standsektor die umliegenden Sektoren, die ich
    ** betreten kann. Davon waehle ich nach Wertung den besten und stelle
    ** mich auf den. Den Standsektor klinke ich dann in die close_list
    ** ein.
    **
    ** Ausserdem klinken wir jede OpenNode in einen Baum ein. Ein Knoten
    ** ist der Sektor, auf dem wir stehen und die Zweige sind alle Offenen
    ** Nodes. Die doppelte Verwaltung ist folgendermassen begruendet:
    **      - die openlist dient dazu, schnell alle Open-Sektoren zu finden,
    **        um einen neuen Sektor auswaehlen zu koennen.
    **      - Der Baum dient dazu, einen aufgebauten, sich verzweigenden
    **        Weg rueckwaerts durchgehen zu koennen (also um den einzigen,
    **        durchfuehrenden Weg zu finden).
    ** -------------------------------------------------------------------*/
    while( 1 ) {

        /* ---------------------------------------------------------------
        ** Wir stehen auf einem Sektor. Alle umliegenden Sektoren,
        ** die nicht in der Openlist oder der Closelist sind, untersuchen,
        ** denn die sind noch frei zum betreten.
        ** Alle betretbaren werten und einklinken.
        ** -------------------------------------------------------------*/
        for( x = -1; x < 2; x++ ) {
            for( y = -1; y < 2; y++ ) {

                FLOAT f, g;

                /*** eigener Sektor? ***/
                if( (x==0) && (y==0) )
                    continue;

                /*** Existiert Sektor? ***/
                if( ((standing_x + x) > 0) &&
                    ((standing_x + x) < (ybd->bact.WSecX-1)) &&
                    ((standing_y + y) > 0) &&
                    ((standing_y + y) < (ybd->bact.WSecY-1)) ) {

                    /*** Sektor holen ***/
                    sector = yb_GetSector( ywd, standing_x + x, standing_y + y);

                    /*** noch frei? ***/
                    //if( sector->PathFlags & (SPF_InOpen | SPF_InClose) )
                    //    continue;
                    if( sector->PathFlags & (SPF_InClose) )
                        continue;

                    /*** Verboten? ***/
                    if( sector->Obstacle >= SPV_Forbidden )
                        continue;

                    /*** Abhang? ***/
                    if( fabs( standing_sector->Height - sector->Height ) >=
                        SPV_WallHeight )
                        continue;
                        
                    /*** verbotener sektor (kompakt und extra koll.skeleton) ***/
                    if( (SECTYPE_COMPACT == sector->SType) &&
                        (sector != to_sector) ) {
                    
                        int nr;
                        if( (nr = GET_LEGONUM( ywd, sector, 0, 0 )) &&
                            (ywd->Legos[nr].altsklt != ywd->Legos[nr].colsklt) )
                            continue;
                        }

                    /* --------------------------------------------------
                    ** kritische Diagonale? Kritisch ist es genau dann,
                    ** wenn die Hoehendiff. von einem zu einem anderen
                    ** der 4 beteiligten Sektoren kritisch ist. Denn dann
                    ** ist der CrossSlurp zu steil, egal in welche
                    ** Richtung ich fahre.
                    ** Auch, wenn ich mit Kanonen auf Spatzen schiesse,
                    ** ich nehme ALLE 4 Sektoren!
                    ** ------------------------------------------------*/
                    if( (x != 0) && (y != 0) ) {

                        /*** Ein DiagonalSektor ***/
                        struct Cell *ds1, *ds2;

                        ds1 = yb_GetSector( ywd, standing_x, standing_y + y);
                        ds2 = yb_GetSector( ywd, standing_x + x, standing_y);

                        /*** Wie sind die Hoehenunterschiede ***/
                        if( (fabs(standing_sector->Height-sector->Height)>SPV_CritHeight) ||
                            (fabs(standing_sector->Height-ds1->Height)>SPV_CritHeight) ||
                            (fabs(standing_sector->Height-ds2->Height)>SPV_CritHeight) ||
                            (fabs(ds1->Height            -ds2->Height)>SPV_CritHeight) ||
                            (fabs(sector->Height         -ds1->Height)>SPV_CritHeight) ||
                            (fabs(sector->Height         -ds2->Height)>SPV_CritHeight) )
                            continue;
                        }

                    /*** ok, dann werten ***/
                    f = yb_f( standing_x, standing_y,
                              standing_sector,
                              standing_x + x, standing_y + y,
                              sector);
                    g = yb_g( standing_x + x, standing_y + y,
                              sec_to_x, sec_to_y );

                    /* ----------------------------------------
                    ** Sektoren, die schon gewertet wurden, nur
                    ** uebernehmen, wenn sie besser sind, also
                    ** die Summer kleiner ist.
                    ** Sonst Sektor ignorieren, auch nicht um-
                    ** schichten.
                    ** --------------------------------------*/
                    if( (sector->PathFlags & SPF_InOpen) &&
                        ((f + g) > (sector->FPathValue+sector->GPathValue)) )
                        continue;

                    /*** Werte uebernehmen ***/
                    sector->FPathValue = f;
                    sector->GPathValue = g;

                    /*** und in OpenList einklinken ***/
                    if( !(sector->PathFlags & SPF_InOpen) ) {

                        _AddTail( (struct List *) &(open_list),
                                  (struct Node *) &(sector->PathNode) );
                        }

                    /*** Baum bauen. Zweig sektor an Knoten standing_sector ***/
                    if( sector->PathFlags & SPF_InOpen )
                        _Remove( (struct Node *) &(sector->TreeNode) );

                    _AddTail( (struct List *) &(standing_sector->TreeList),
                              (struct Node *) &(sector->TreeNode) );
                    sector->TreeUp = standing_sector;

                    /*** markieren ***/
                    sector->PathFlags |= SPF_InOpen;
                    }
                }
            }

        /* ------------------------------------------------------------------
        ** Nun kann es passieren, dass die OpenList leer ist. Das heisst,
        ** wir kommen nie und nimmer zum Ziel. Also verlassen wir die Routine
        ** mit FALSE und num_waypoints = 0.
        ** ----------------------------------------------------------------*/
        if( open_list.mlh_Head->mln_Succ == NULL ) {
             fpath->num_waypoints = 0;
             return( FALSE );
             }

        /* --------------------------------------------------------------
        ** Nun haben wir eine aktualisierte OpenList. Aus dieser suchen
        ** wir einen neuen Sektor heraus, und zwar den mit dem geringsten
        ** PathValue (F+G).
        ** Dieser kommt auf jeden fall in die OpenList. Sollte das schon
        ** der Zielsektor sein, verlasse ich die while-Schleife
        ** ------------------------------------------------------------*/
        running_sector = (struct Cell *) open_list.mlh_Head;
        sector         = running_sector;
        value          = sector->FPathValue + sector->GPathValue;

        while( running_sector->PathNode.mln_Succ ) {

            FLOAT nv = running_sector->FPathValue + running_sector->GPathValue;

            /*** Besser? ***/
            if( nv < value ) {
                value  = nv;
                sector = running_sector;
                }

            running_sector = (struct Cell *) running_sector->PathNode.mln_Succ;
            }

        /*** sector ist der neue "standing"... ***/
        standing_sector = sector;
        standing_x      = sector->PosX;
        standing_y      = sector->PosY;
        _NewList( (struct List *) &(standing_sector->TreeList) );

        /* ----------------------------------------------------------
        ** Als Closed markieren. Das heisst, aus der open_list, nicht
        ** aber aus dem Baum rausnehmen!
        ** --------------------------------------------------------*/
        _Remove(  (struct Node *) &(sector->PathNode) );
        sector->PathFlags &= ~SPF_InOpen;
        sector->PathFlags |=  SPF_InClose;

        /*** das Ziel? ***/
        if( (standing_x == sec_to_x) &&
            (standing_y == sec_to_y) )
            break;
        }


    /* -------------------------------------------------------------------
    ** wir haben einen Weg. Ich gehe nun vom Ziel rueckwaerts zum Anfang
    ** den Baum hoch. Dazu suche ich die Node zum Zielpunkt und hangele
    ** mich an Hand der Up-Pointer wieder hoch. Jede Node, die ich so finde,
    ** packe ich in ein way_list. Das macht es mir einfach, daraus dann
    ** die Teilziele zusammenzubasteln.
    ** -----------------------------------------------------------------*/
    _NewList( (struct List *) &(way_list) );
    sector = yb_GetSector( ywd, sec_to_x, sec_to_y );
    while( sector ) {

        _AddHead( (struct List *) &(way_list),
                  (struct Node *) &(sector->PathNode) );
        sector = sector->TreeUp;
        }


    /* -----------------------------------------------------------------
    ** So, auf zum letzten Streich. In way_list ist jetzt eine Menge
    ** von Nodes, die den Weg zum Ziel beschreiben. Nun muessen wir
    ** daraus (begrenzt durch die Anzahl freier Waypoints) eine menge
    ** von Teilzielen machen.
    ** Jedesmal, wenn wir die Richtung aendern muessen, wird der "Knick-
    ** punkt" ein Waypoint. Sind die Waypoints alle, wird einfach der
    ** letzte eintrag der liste das letzte Ziel.
    ** ---------------------------------------------------------------*/
    sector      = (struct Cell *) way_list.mlh_Head;
    next_sector = (struct Cell *) sector->PathNode.mln_Succ;
    dir_x       = next_sector->PosX - sector->PosX;
    dir_y       = next_sector->PosY - sector->PosY;
    wp          = 0;

    while( next_sector->PathNode.mln_Succ ) {

        /* -------------------------------------------------------------
        ** Sind die Wegpunkte alle, dann lohnt es sich gar nicht, weiter
        ** nachzugucken, denn der letzte ist eh der Zielpunkt.
        ** -----------------------------------------------------------*/
        if( num_waypoints <= 1 ) {
            fpath->waypoint[ wp ].x = fpath->to_x;
            fpath->waypoint[ wp ].z = fpath->to_z;
            break;
            }

        /*** naechster schon das Ziel? ***/
        if( next_sector == to_sector ) {
            fpath->waypoint[ wp ].x = fpath->to_x;
            fpath->waypoint[ wp ].z = fpath->to_z;
            break;
            }

        /*** Einen Schritt vorwaerts ***/
        sector      = next_sector;
        next_sector = (struct Cell *) next_sector->PathNode.mln_Succ;

        /*** Aendert sich Richtung? ***/
        if( (dir_x != (next_sector->PosX - sector->PosX)) ||
            (dir_y != (next_sector->PosY - sector->PosY)) ) {

            FLOAT versatz_x, versatz_z;

            /* ------------------------------------------------------
            ** Zielpunkt wegen kompakten Gebaeuden etwas verschieben.
            ** Ich bleibe auf der Seite, von der ich komme.
            ** ----------------------------------------------------*/
            if( abs(dir_x) >= abs(dir_y) ) {

                /*** in ost-west-Richtung ***/
                if( dir_y > 0 ) {

                    /*** in der Map nach unten, also oben bleiben ***/
                    versatz_x = 0;
                    versatz_z = 200;
                    }
                else {

                    /*** nach oben, also den unteren Punkt nehmen ***/
                    versatz_x = 0;
                    versatz_z = -200;
                    }
                }
            else {

                /*** in nord-sued-Richtung ***/
                if( dir_x > 0 ) {

                    /*** nach rechts, also links bleiben ***/
                    versatz_x = -200;
                    versatz_z = 0;
                    }
                else {

                    /*** rechts bleiben ***/
                    versatz_x = 200;
                    versatz_z = 0;
                    }
                }

            /*** Richtung aendern ***/
            dir_x = next_sector->PosX - sector->PosX;
            dir_y = next_sector->PosY - sector->PosY;

            /*** und sector wird ein Zwischenziel ***/
            fpath->waypoint[ wp ].x  =  ((FLOAT)sector->PosX + 0.5) * SECTOR_SIZE;
            fpath->waypoint[ wp ].z  = -((FLOAT)sector->PosY + 0.5) * SECTOR_SIZE;
            fpath->waypoint[ wp ].x += versatz_x;
            fpath->waypoint[ wp ].z += versatz_z;

            num_waypoints--;
            wp++;
            }

        /*** Hochzaehlen ist in der Schleife ***/
        }

    /* -------------------------------------------------------
    ** listen fuer Tests aufraeumen. Das fliegt spaeter wieder
    ** raus.
    ** -----------------------------------------------------*/
    while( way_list.mlh_Head->mln_Succ )
        _Remove( (struct Node *) way_list.mlh_Head );
    while( open_list.mlh_Head->mln_Succ )
        _Remove( (struct Node *) open_list.mlh_Head );

    /*** Tatsaechliche Wegpunkte zurueckgeben ***/
    fpath->num_waypoints = wp + 1;

    return( TRUE );
}


struct Cell *yb_GetSector( struct ypaworld_data *ywd, WORD pos_x, WORD pos_y)
{
    /* --------------------------------------------------------------------
    ** Ermittelt aus dem Cell-Array an Hand der postition den Sector und
    ** schreibt auchg gleich die Postion mal mit rein, weil wir die haeufig
    ** brauchen.
    ** ------------------------------------------------------------------*/
    struct Cell *sector;

    if( (pos_x<0) || (pos_y<0) || (pos_x>=ywd->MapSizeX) || (pos_y>=ywd->MapSizeY) )
        return( NULL );

    sector = &( ywd->CellArea[ ywd->MapSizeX * pos_y + pos_x ]);
    sector->PosX = pos_x;
    sector->PosY = pos_y;

    return( sector );
}


FLOAT yb_f( WORD from_x, WORD from_y, struct Cell *from_sector,
            WORD to_x, WORD to_y, struct Cell *to_sector )
{
    /* -----------------------------------------------------------------
    ** f ermittelt die Wertigkeit des bisherigen Wertes. Dabei gehen wir
    ** von einem bereits gewerteten Sector (auf dem wir stehen) zu einem
    ** Nachbarsektor. Das heisst: Wert von Standing + Wert von Weg zu
    ** diesem Sektor.
    ** Hier beachten wir auch die spezielle Wertigkeit des Sektors
    ** ---------------------------------------------------------------*/
    FLOAT value;
    WORD  dx, dy;

    /*** Bisher ***/
    value  = from_sector->FPathValue;

    /*** + Hindernis ***/
    value += (FLOAT)to_sector->Obstacle;

    /*** + Weg ***/
    dx = from_x - to_x;
    dy = from_y - to_y;
    value += sqrt( (FLOAT)( dx * dx ) + (FLOAT)( dy * dy ) );

    return( value );
}


FLOAT yb_g( WORD from_x, WORD from_y, WORD to_x, WORD to_y )
{
    /* -----------------------------------------------------------------
    ** Wertet die Strecke von hier bis zum Ziel. dabei beachten wir nur
    ** die Entfernung fuer den guenstigsten Fall. Das waere erstmal dia-
    ** gonal zu gehen, bis ich an einen rand komme, und dann diagonal.
    ** das heisst auf deutsch:
    ** min( dx, dy ) * sqrt( 2 ) + abs( dx - dy )
    ** Alles klar?
    ** ---------------------------------------------------------------*/
    WORD dx, dy;
    FLOAT value;

    dx = abs( to_x - from_x );
    dy = abs( to_y - from_y );

    value  = sqrt( 2.0 ) * (FLOAT)min( dx, dy );
    value += (FLOAT) abs( dx - dy );

    return( value );
}


void yb_GiveWayPointToSlaves( struct Bacterium *bact, WORD count )
{
    /* ----------------------------------------------------------
    ** kopiert aktuelle Wegpunktsituation, also num_waypoints und
    ** waypoint, nicht aber count_waypoints. Wenn es aber der
    ** erste Wegpunkt war, num_waypoints also 1 ist, dann setze
    ** ich den count schon auf 0.
    ** --------------------------------------------------------*/
    struct OBNode *slave;

    for( slave = (struct OBNode *) bact->slave_list.mlh_Head;
         slave->nd.mln_Succ;
         slave = (struct OBNode *)  slave->nd.mln_Succ ) {

        int i;

        slave->bact->num_waypoints = bact->num_waypoints;

        slave->bact->count_waypoints = count;

        slave->bact->ExtraState   |= EXTRA_DOINGWAYPOINT;

        if( bact->ExtraState & EXTRA_WAYPOINTCYCLE )
            slave->bact->ExtraState |=  EXTRA_WAYPOINTCYCLE;
        else
            slave->bact->ExtraState &= ~EXTRA_WAYPOINTCYCLE;

        for( i = 0; i < MAXNUM_WAYPOINTS; i++ )
            slave->bact->waypoint[ i ] = bact->waypoint[ i ];
        }
}

