/*
**  $Source: $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:24:47 $
**  $Locker: $
**  $Author: $
**
**  Die Intelligenzen...
**
**  c Copyright 1995 by Andreas Flemming
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
#include "ypa/yparoboclass.h"
#include "ypa/ypavehicles.h"
#include "input/input.h"


/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine

extern ULONG YPA_CommandCount;

BOOL yr_FindRobo( struct yparobo_data *yrd, struct Bacterium *robo);

/*-----------------------------------------------------------------*/


_dispatcher( void, yr_YRM_GETCOMMANDER, struct getcommander_msg *gc )
{
/*
**  FUNCTION    Gibt einen Commander, so es einen findet, zurück, der
**              den übergebenen Informationen genügt
**
**  INPUT       CommanderID, oder Projektnummer oder....
**
**  RESULT      bact und Object
**
**  CHANGED     31-Oct-95 kriäted 8100 000C
**              12-Nov-95 dodalst erweitert
*/

    struct yparobo_data *yrd;
    struct OBNode       *com;
    struct MinList      *list;

    BOOL found_PrimaryTarget, found_CommandID, found_SectorDistance;

    yrd = INST_DATA( cl, o );
    gc->com     = NULL;
    gc->combact = NULL;

    list = &(yrd->bact->slave_list);
    com = (struct OBNode *) list->mlh_Head;

    while( com->nd.mln_Succ ) {

        /*** keene Leichen zurückgeben! ***/
        if( com->bact->ExtraState & EXTRA_LOGICDEATH ) {

            com = (struct OBNode *) com->nd.mln_Succ;
            continue;
            }


        /*** CommandID ***/
        if( gc->flags & GC_COMMANDID ) {

            if( com->bact->CommandID == gc->CommandID ) {

                gc->combact = com->bact;
                gc->com     = com->o;
                found_CommandID = TRUE;
                }
            else
                found_CommandID = FALSE;
            }
        else
            found_CommandID = TRUE;


        /*** Primarytarget ***/
        if( gc->flags & GC_PRIMTARGET ) {

            if( (com->bact->PrimTargetType == gc->target_type) ||
                (yrd->dttype == gc->target_type) ) {

                /* ------------------------------------------------
                ** Wenn die PrimCommandID 0 ist, dann nehme ich zum
                ** Vergleich den Bakterienpointer. Ich denke (na-
                ** türlich wie immer zu spät) daran, daß das Ziel
                ** auch im Dock stehen kann, wenn derjenige als
                ** DockUser eingetragen ist!
                ** ----------------------------------------------*/
                WORD  tarsec_x, tarsec_y, gcsec_x, gcsec_y;
                LONG  tarid;

                gcsec_x  = (WORD)(gc->targetpos.x  / SECTOR_SIZE);
                gcsec_y  = (WORD)(-gc->targetpos.z / SECTOR_SIZE);

                if( (com->bact->CommandID == yrd->dock_user) &&
                    (yrd->dock_user != 0) ) {
                    tarsec_x = (WORD)( yrd->dtpos.x / SECTOR_SIZE);
                    tarsec_y = (WORD)(-yrd->dtpos.z / SECTOR_SIZE);
                    tarid    = yrd->dtCommandID;
                    }
                else {
                    tarsec_x = (WORD)( com->bact->PrimPos.x / SECTOR_SIZE);
                    tarsec_y = (WORD)(-com->bact->PrimPos.z / SECTOR_SIZE);
                    tarid    = com->bact->PrimCommandID;
                    }

                /*** Nun mal testen ***/
                if( ((gc->target_type == TARTYPE_BACTERIUM) &&
                     (gc->PrimCommandID == 0) &&
                     (gc->target.bact == com->bact->PrimaryTarget.Bact)) ||
                    
                    ((gc->target_type == TARTYPE_BACTERIUM) &&
                     (gc->PrimCommandID != 0) &&
                     (gc->PrimCommandID == tarid)) ||

                    ((gc->target_type == TARTYPE_SECTOR) &&
                     ( tarsec_x == gcsec_x ) &&
                     ( tarsec_y == gcsec_y ) ) ) {

                    gc->combact = com->bact;
                    gc->com     = com->o;
                    found_PrimaryTarget = TRUE;
                    }
                found_PrimaryTarget = FALSE;
                }
            else
                found_PrimaryTarget = FALSE;
            }
        else
            found_PrimaryTarget = TRUE;


        /*** Entfernung zum PrimSECTOR(!)target, nicht bloß sector selbst ***/
        if( gc->flags & GC_PRIMSECTORDIST ) {

            if( com->bact->PrimTargetType == TARTYPE_SECTOR) {

                /* ----------------------------------------------
                ** Wir untersuchen nur die Entfernung zum Sektor-
                ** target, weil es nur da Sinn hat...
                ** --------------------------------------------*/
                FLOAT tarpos_x, tarpos_z, gcpos_x, gcpos_z;

                gcpos_x  = gc->targetpos.x;
                gcpos_z  = gc->targetpos.z;

                if( (com->bact->CommandID == yrd->dock_user) &&
                    (yrd->dock_user != 0) ) {
                    tarpos_x = yrd->dtpos.x;
                    tarpos_z = yrd->dtpos.z;
                    }
                else {
                    tarpos_x = com->bact->PrimPos.x;
                    tarpos_z = com->bact->PrimPos.z;
                    }

                /*** Nun mal testen ***/
                if( ((tarpos_x - gcpos_x) * (tarpos_x - gcpos_x) +
                     (tarpos_z - gcpos_z) * (tarpos_z - gcpos_z)) <
                     gc->distance * gc->distance ) {

                    gc->combact = com->bact;
                    gc->com     = com->o;
                    found_SectorDistance = TRUE;
                    }
                else
                    found_SectorDistance = FALSE;
                }
            else
                found_SectorDistance = FALSE;
            }
        else
            found_SectorDistance = TRUE;

        /*** Gefunden? ***/
        if( found_CommandID && found_PrimaryTarget && found_SectorDistance )
            return;


        com = (struct OBNode *) com->nd.mln_Succ;
        }
}


_dispatcher( void, yr_YRM_ALLOWVTYPES, ULONG *types )
{
/*
**  FUNCTION    Erlaubt das Bauen der übergebenen Typen
**
**  INPUT       Array mit VTypes
**
**  RESULT      ...
**
**  CHANGED     1-Nov-95  8100 000C created
*/

    struct yparobo_data *yrd;
    struct VehicleProto *vp_array;
    UBYTE  footprint;
    ULONG  i;

    yrd = INST_DATA( cl, o );
    i = 0;
    
    /*** Liste holen ***/
    _get( yrd->world, YWA_VehicleArray, &vp_array );

    while( types[ i ] != VTYPE_DONE ) {

        footprint = (UBYTE)( 1 << yrd->bact->Owner );
        vp_array[ types[ i ] ].FootPrint |= footprint;

        i++;
        }
}


_dispatcher( void, yr_YRM_FORBIDVTYPES, ULONG *types )
{
/*
**  FUNCTION    Verbietet das Bauen der übergebenen Typen
**
**  INPUT       Array mit VTypes
**
**  RESULT      ...
**
**  CHANGED     1-Nov-95  8100 000C created
*/

    struct yparobo_data *yrd;
    struct VehicleProto *vp_array;
    UBYTE  footprint;
    ULONG  i;

    yrd = INST_DATA( cl, o );
    i = 0;

    /*** Liste holen ***/
    _get( yrd->world, YWA_VehicleArray, &vp_array );

    while( types[ i ] != VTYPE_DONE ) {

        footprint = (UBYTE)( 1 << yrd->bact->Owner );
        vp_array[ types[ i ] ].FootPrint &= ~footprint;

        i++;
        }
}



_dispatcher( BOOL, yr_YRM_SEARCHROBO, struct searchrobo_msg *sr )
{
/*
**  FUNCTION    Sucht Robo (feindlich!) entsprechend den Flags im
**              gesamten Bereich
**
**              Flags
**
**  RESULT      struct Bacterium *robo ist gültig, wenn ret == TRUE
**              (nachdem gesamter Bereich gescannt wurde)
**              ret == FALSE && Pointer == 1L, dann ist keiner drin,
**              ansonsten kann trotzdem einer drin sein
**
**  CHANGED     5-11-95    8100 000C     created
**             21-11-95    8100 000C     hier kann man doch die Liste nehmen !!!
*/

    struct yparobo_data *yrd;
    FLOAT  merke_dist, distance;
    struct MinNode *node, *erster;
    struct MinList *list;
    struct OBNode *kandidat, *robo;
    struct flt_triple weg;


    yrd = INST_DATA( cl, o );

    /*** ListenHeader suchen ***/
    node = (struct MinNode *) &(yrd->bact->slave_node);
    while( node->mln_Pred )
           node = node->mln_Pred;
    list = (struct MinList *) node;

    /*** Header merken ***/
    erster = list->mlh_Head;
    if( ((struct OBNode *)erster)->o == o ) {

        /*** Bin nur ich drin? ***/
        erster = erster->mln_Succ;
        if( erster->mln_Succ == NULL ) return( FALSE );
        }
    kandidat = NULL;

    /*** Wenn keine Wünsche übergeben wurden, gibts gleich den zurück ***/
    if( sr->flags == 0L ) {

        sr->found_x  = ((struct OBNode *)node)->bact->pos.x;
        sr->found_z  = ((struct OBNode *)node)->bact->pos.z;
        sr->robo     = ((struct OBNode *)node)->bact;
        sr->lastrobo = ((struct OBNode *)node)->bact;
        return( TRUE );
        }

    /*** erster ist jetzt der erste Feind in der Liste. Nun suchen wir ***/
    node = list->mlh_Head;
    while( node->mln_Succ ) {

        robo = (struct OBNode *)node;

        /*** Bin ich dat? ***/
        if( robo->o == o ) {

            node = node->mln_Succ;
            continue;
            }

        /*** Ist das ein Füllelement? ***/
        if( robo->bact->BactClassID != BCLID_YPAROBO ) {
                
            node = node->mln_Succ;
            continue;
            }

        /*** Kann ich den sehen? ***/
        if( (sr->flags & SR_KNOWNTERR) && (robo->bact->Owner < 8 ) ) {

            UBYTE mask = 1 << yrd->bact->Owner;

            if( !(mask & robo->bact->Sector->FootPrint) ) {

                /*** den können wir nicht mitkriegen ***/
                node = node->mln_Succ;
                continue;
                }
            }

        /*** Ist es eigenes gebiet? ***/
        if( sr->flags & SR_OWNTERR ) {

            if( yrd->bact->Owner != robo->bact->Sector->Owner ) {

                /*** den können wir nicht mitkriegen ***/
                node = node->mln_Succ;
                continue;
                }
            }


        /*** Weiter untersuchen nach Entfernung ODER Liste ***/
        if( sr->flags & SR_NEXT ) {

            weg.x = robo->bact->pos.x - yrd->bact->pos.x;
            weg.y = robo->bact->pos.y - yrd->bact->pos.y;
            weg.z = robo->bact->pos.z - yrd->bact->pos.z;
            distance = nc_sqrt( weg.x*weg.x + weg.y*weg.y + weg.z*weg.z);

            if( kandidat == NULL ) {

                kandidat   = robo;
                merke_dist = distance;
                }
            else {

                if( merke_dist > distance ) {

                    merke_dist = distance;
                    kandidat   = robo;
                    }
                }
            }
        else {

            if( sr->flags & SR_SUCC ) {

                /*** Den obersten haben wir auf jeden Fall, wenn lr == NULL ***/
                if( (kandidat == NULL) && (sr->lastrobo == NULL) )
                     kandidat = (struct OBNode *)erster;

                /*** Ist node zufällig lastrobo? ***/
                if( (robo->bact == sr->lastrobo) && (robo->nd.mln_Succ) ) {

                    /*** tütütütü - tütütütü - Hoppla! ***/
                    kandidat = (struct OBNode *)robo->nd.mln_Succ;
                    }
                }
            }

        /*** Nachfolger holen ***/
        node = node->mln_Succ;
        }

    if( kandidat ) {

        /*** Mir ham was gefunden ***/
        sr->robo     = kandidat->bact;
        sr->lastrobo = kandidat->bact;
        sr->found_x  = kandidat->bact->pos.x;
        sr->found_z  = kandidat->bact->pos.z;
        return( TRUE );
        }
    else {

        /*** Mir ham nix gefunden ***/
        return( FALSE );
        }
}


_dispatcher( BOOL, yr_YRM_GETENEMY, struct settarget_msg *msg)
{
/*
**  FUNCTION    Sehr spezielle Methode: Liefert einen Commanderführer
**              zu einer CommandID, die in target.priority übergeben wird.
**              Ich nehme deshalb die settarget_msg, weil man gleich danach
**              sicherlich ein Ziel setzen will. Wenn der Bact-Pointer
**              richtig ist (also was gefunden wurde), dann gebe ich TRUE
**              zurück.
**              Achtung, auch die Robos müssen untersucht werden!!!
**
**  RESULTS     TRUE: für ein BakterienHauptziel ausgefüllte settarget_msg
**
**  INPUT       CommandID in priority
**
**  CHANGED     heute hab ich das erstmal geschrieben
*/

    struct yparobo_data *yrd;
    struct MinList *list;
    struct MinNode *node;
    struct OBNode  *robo;
    struct OBNode  *slave;
    struct MinList *slist;

    yrd = INST_DATA( cl, o);

    /*** Was sinnvolles? ***/
    if( msg->priority == 0 ) {
    
        msg->priority    = 0;
        msg->target_type = TARTYPE_NONE;
        return( FALSE );
        }

    /*** Zuerst ListenHeader suchen ***/
    node = (struct MinNode *) &(yrd->bact->slave_node);
    while( node->mln_Pred ) node = node->mln_Pred;
    list = (struct MinList *) node;

    /*** Nun alle Robos der Liste scannen ***/
    node = list->mlh_Head;
    while( node->mln_Succ ) {

        robo = (struct OBNode *) node;

        /*** Kann es der Robo sein? ***/
        if( msg->priority == robo->bact->CommandID ) {

            /*** Gefunden (Wenn nicht tot) !!! ***/
            if( ACTION_DEAD != robo->bact->MainState ) {

                /*** Er ist's und lebt ***/
                msg->target.bact = robo->bact;
                msg->priority    = 0;
                msg->target_type = TARTYPE_BACTERIUM;
                return( TRUE );
                }
            else {

                /*** Das war er mal... ***/
                msg->target_type = TARTYPE_NONE;
                msg->priority    = 0;
                return( FALSE );
                }
            }


        /*** Scannen wir nun seine SlaveList ***/
        slist = &(robo->bact->slave_list);
        slave = (struct OBNode *) slist->mlh_Head;
        while( slave->nd.mln_Succ ) {

            if( msg->priority == slave->bact->CommandID ) {

                /*** Gefunden!!! ***/
                if( ACTION_DEAD != slave->bact->MainState ) {

                    /*** Jo, alles ok ***/
                    msg->target.bact = slave->bact;
                    msg->priority    = 0;
                    msg->target_type = TARTYPE_BACTERIUM;
                    return( TRUE );
                    }
                else {

                    /*** Hoppla, das geschwader gibts gar nicht mehr ***/
                    msg->target_type = TARTYPE_NONE;
                    msg->priority    = 0;
                    return( FALSE );
                    }
                }

            /*** next slave ***/
            slave = (struct OBNode *) slave->nd.mln_Succ;
            }

        /*** next Robo ***/
        node = node->mln_Succ;
        }

    /*** Dann haben wir nix gefunden ***/
    msg->priority    = 0;
    msg->target_type = TARTYPE_NONE;
    
    return( FALSE );
}



_dispatcher( BOOL, yr_YRM_EXTERNMAKECOMMAND, struct externmakecommand_msg *emc )
{
/*
**  FUNCTION    Erzeugt ein Geschwader und ordnet es dem Robo zu, so können dem
**              Robo vergefertigte Geschwader gegeben werden. dabei kann ich festlegen,
**              ob die Geschwader in den UnUsable-State sollen (dann werden sie für
**              die Arbeit solange mißachtet, bis mal jemand das Fluchtflag setzt.
**              die übergebene Position wird zum Hauptziel, so daß die Leute
**              "die Stellung halten".
**
**              Erweiterung: Wenn ein Array abgegeben wird, so werden die
**              Prototypen aus diesem feld genommen, wobei der erste der
**              Commander wird.
**
**              Das EVER-Flag wird NICHT mehr unterstützt
**
**              Im Netzwerkmodus gibt es sowas nicht, deshalb keine
**              Modifizierung der CommandID
**
**  INPUT       position
**              VehicleProto
**              Flags:  UNUSABLE    Alle (!) Leute für Robo sperren
**
**  RESULT      Hat geklappt (wenn auch nicht vollständig)  -->TRUE
**
**  CHANGED     Wen interessiert denn das? Was soll das alles?
*/

    struct yparobo_data *yrd;
    struct VehicleProto *vp_array;
    WORD   wieviel;
    struct createvehicle_msg cv;
    struct setstate_msg state;
    struct settarget_msg target;
    Object *Commander, *Slave;
    struct Bacterium *bact;
    ULONG  bc;
    FLOAT  angle;
    WORD   x, raster, count;

    yrd = INST_DATA( cl, o);
    wieviel = emc->number;

    /*** Sinnlose Werte? ***/
    //if( emc->number ) was soll das?

    /*** Die Vehicle-Liste holen ***/
    _get( yrd->world, YWA_VehicleArray, &vp_array );

    /*** Commander erzeugen ***/
    x      = (WORD) nc_sqrt( emc->number ) + 2;
    raster = 50; 
    count  = 0;    
    cv.y   = emc->pos.y;
    
    while( 1 ) {
    
        struct getsectorinfo_msg gsi;
        struct intersect_msg inter;
    
        cv.x = emc->pos.x + (count % x - x/2) * raster;
        cv.z = emc->pos.z + (count / x) * raster;
        count++;
        
        /*** guenstige Abwurfposition ? ***/
        gsi.abspos_x = cv.x;
        gsi.abspos_z = cv.z;
        _methoda( yrd->world, YWM_GETSECTORINFO, &gsi );
        
        inter.pnt.x = cv.x;
        inter.pnt.y = cv.y - 10000.0;
        inter.pnt.z = cv.z;
        inter.vec.x = inter.vec.z = 0.0;
        inter.vec.y = 20000.0;
        inter.flags = INTERSECTF_CHEAT;
        _methoda( yrd->world, YWM_INTERSECT, &inter );
        
        if( inter.insect && 
            ( fabs(inter.ipnt.y - gsi.sector->Height) < 30.0) )
            break;
        }

    if( emc->varray )
        cv.vp = (UBYTE)(emc->varray[ 0 ]);
    else
        cv.vp = emc->vproto;

    if( !(Commander = (Object *) _methoda( yrd->world, YWM_CREATEVEHICLE, &cv ))) 
        return( FALSE );
    
    /*** Initialisieren ***/
    _get( Commander, YBA_Bacterium, &bact );
    _get( o, YBA_BactCollision, &bc );
    _set( Commander, YBA_BactCollision, bc );
    if( emc->flags & EMC_UNUSABLE ) bact->ExtraState |= EXTRA_UNUSABLE;
    bact->Owner      = yrd->bact->Owner;
    bact->CommandID  = YPA_CommandCount;
    bact->robo       = o;
    bact->Aggression = 60;
    state.extra_on   = state.extra_off = 0;
    state.main_state = ACTION_NORMAL;
    _methoda( Commander, YBM_SETSTATE, &state );
    wieviel--;
    
    /*** bleib dort stehen ***/
    target.priority    = 0;
    target.target_type = TARTYPE_SECTOR;
    target.pos.x       = cv.x;
    target.pos.z       = cv.z;
    _methoda( Commander, YBM_SETTARGET, &target );
     

    while( wieviel ) {

        if( emc->varray )
            cv.vp = (UBYTE)(emc->varray[ wieviel ]);
        else
            cv.vp = emc->vproto;

        /* ------------------------------------------------------
        ** Rechteck mit Grundseite sqrt( anzahl)+1 mit pos in der
        ** mitte. Von da aus nach vorn
        ** ----------------------------------------------------*/
        while( 1 ) {
        
            struct getsectorinfo_msg gsi;
            struct intersect_msg inter;
            BOOL    ret;
        
            cv.x = emc->pos.x + (count % x - x/2) * raster;
            cv.z = emc->pos.z + (count / x) * raster;
            count++;
            
            /*** guenstige Abwurfposition ? ***/
            gsi.abspos_x = cv.x;
            gsi.abspos_z = cv.z;
            ret = _methoda( yrd->world, YWM_GETSECTORINFO, &gsi );
            
            inter.pnt.x = cv.x;
            inter.pnt.y = cv.y - 10000.0;
            inter.pnt.z = cv.z;
            inter.vec.x = inter.vec.z = 0.0;
            inter.vec.y = 20000.0;
            inter.flags = INTERSECTF_CHEAT;
            _methoda( yrd->world, YWM_INTERSECT, &inter );
            
            if( inter.insect && 
                ret &&
                ( fabs(inter.ipnt.y - gsi.sector->Height) < 30.0) )
                break;
            }

        if( !(Slave = (Object *) _methoda( yrd->world, YWM_CREATEVEHICLE, &cv ))) 
            break;
        
        /*** Ankoppeln ***/
        _methoda( Commander, YBM_ADDSLAVE, Slave );

        target.pos.x       = cv.x;
        target.pos.z       = cv.z;
        _methoda( Slave, YBM_SETTARGET, &target );

        /*** Initialisieren ***/
        _get( Slave, YBA_Bacterium, &bact );
        _set( Slave, YBA_BactCollision, bc );
        if( emc->flags & EMC_UNUSABLE ) bact->ExtraState |= EXTRA_UNUSABLE;
        bact->Owner     = yrd->bact->Owner;
        bact->CommandID = YPA_CommandCount;
        bact->robo      = o;
                bact->Aggression = 60;
        _methoda( Slave, YBM_SETSTATE, &state );

        /*** erst hier reduzieren ***/
        wieviel--;
        }

    /*** Erhöhen ***/
    YPA_CommandCount++;

    /*** an Robo ***/
    _methoda( o, YBM_ADDSLAVE, Commander );

    return( TRUE );
}

void yr_HistoryCreate( Object *world, struct Bacterium *new )
{
    struct ypa_HistVhclCreate vcreate;

    /*** Zeugungsmitteilung für das statistische Amt ***/
    vcreate.cmd    = YPAHIST_VHCLCREATE;
    vcreate.owner  = new->Owner;
    vcreate.vp     = new->TypeID;
    vcreate.pos_x  = (UBYTE)((new->pos.x*256)/new->WorldX);
    vcreate.pos_z  = (UBYTE)((new->pos.z*256)/new->WorldZ);
    _methoda( world, YWM_NOTIFYHISTORYEVENT, &vcreate );
}

