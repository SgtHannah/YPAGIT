/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_building.c,v $
**  $Revision: 38.13 $
**  $Date: 1998/01/06 16:17:20 $
**  $Locker: floh $
**  $Author: floh $
**
**  Building-Zeugs...
**
**  (C) Copyright 1995 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/lists.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "bitmap/bitmapclass.h"
#include "ypa/ypagunclass.h"
#include "ypa/ypaworldclass.h"

#include "yw_protos.h"

#ifdef __NETWORK__
#include "network/networkclass.h"
#include "ypa/ypamessages.h"
#endif

_extern_use_nucleus

/*-----------------------------------------------------------------*/
void yw_BactOverkill(struct ypaworld_data *ywd, ULONG sec_x, ULONG sec_y)
/*
**  FUNCTION
**      Killt alle Bakterien im Sektor <sec>, ausser wenn's ein
**      Robo oder eine Missile ist.
**
**  CHANGED
**      07-Feb-96   floh    created
**      16-Apr-98   floh    + vollstaendig umgeschrieben
**      26-Apr-98   floh    + es werden nur noch Flaks zerstoert
**      02-Jun-98   floh    + ooops, Robo-Flaks konnten gekillt werden...
*/
{
    struct Cell *sec = &(ywd->CellArea[sec_y * ywd->MapSizeX + sec_x]);
    struct MinNode *nd;
    struct MinList *ls = (struct MinList *) &(sec->BactList);
    for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {
        struct Bacterium *b = (struct Bacterium *)nd;
        BOOL do_vehicle = FALSE;
        if (ywd->playing_network) {
            /*** Multiplayer Test-Kriterium ***/
            if ((b->Owner == ywd->UVBact->Owner)  &&
                (b->MainState != ACTION_DEAD)     &&
				(b->MainState != ACTION_BEAM)      &&
				(b->MainState != ACTION_CREATE)    &&
                (b->BactClassID == BCLID_YPAGUN))                
            {
                ULONG is_robo_gun;
                _get(b->BactObject,YGA_RoboGun,&is_robo_gun);
                if (!is_robo_gun) do_vehicle = TRUE;
            };
        } else {
            /*** Singleplayer Test-Kriterium ***/
            if ((b->MainState != ACTION_DEAD)     &&
				(b->MainState != ACTION_BEAM)     &&
				(b->MainState != ACTION_CREATE)   &&
                (b->BactClassID == BCLID_YPAGUN))
            {                
                /*** handelt es sich hier um eine Robo-Gun? ***/
                ULONG is_robo_gun;
                _get(b->BactObject,YGA_RoboGun,&is_robo_gun);
                if (!is_robo_gun) do_vehicle = TRUE;
            };
        };
        if (do_vehicle) {
            struct modvehicleenergy_msg mve;
            mve.energy = -22000000;
            mve.killer = NULL;
            _methoda(b->BactObject,YBM_MODVEHICLEENERGY,&mve);
        };
    };
}

/*-----------------------------------------------------------------*/
void yw_SetSector(Object *world,
                  struct ypaworld_data *ywd,
                  ULONG sec_x, ULONG sec_y,
                  ULONG owner,
                  ULONG bp_num,
                  ULONG no_bacts)
/*
**  FUNCTION
**      "Setzt" den gew¸nschten Buildproto in die Welt.
**      Wird im Immediate-Modus sofort angewendet,
**      im Normal-Bauen-Modus erst wenn der zugehˆrige
**      Bauen-Job zeitlich abgelaufen ist.
**
**      Konkret:
**          - Sektor wird vollst‰ndig neu initialisiert
**          - alle station‰ren Bakterien, die im BuildProto
**            angegeben sind, werden erzeugt und positioniert.
**
**  INPUTS
**      world   - Ptr auf Welt-Object itself
**      ywd     - Ptr auf LID des Welt-Objects
**      sec_x, sec_y    - Sektor-X/Y-Koordinate
**      owner   - auf diesen Owner wird der Sektor gesetzt
**      bp_num  - BuildProto-Nummer
**      no_bacts - wenn TRUE, werden keine attachten Bakterien erzeugt
**
**  RESULTS
**      ---
**
**  CHANGED
**      29-Jan-96   floh    created
**      30-Jan-96   floh    oops, das SubEnergy-Field wurde
**                          nicht voll gelˆscht, so daﬂ es zu
**                          komischen Nebeneffekten kam, wenn
**                          ein Kompakt-Sektor (nur [0][0] g¸ltig)
**                          auf einen ehemaligen 3x3-Sektor gebaut wird.
**      07-Feb-96   floh    + Bakterien werden nicht mehr generell
**                            an den Robo geh‰ngt, sondern bilden ein
**                            "Sektor-Geschwader".
**      10-Feb-96   floh    Guns werden jetzt in GENESIS-Zustand gesetzt
**      14-Feb-96   floh    ƒnderungen von der Milestone2-Version
**                          integriert.
**      11-Mar-96   floh    AF's tempor‰ren WType-Patch ¸bernommen.
**      17-Mar-96   floh    AF's Patch ersetzt durch die "echte" Version,
**                          bei "normalen" Buildings (keine KWs) wird
**                          der WType auf WTYPE_Building gesetzt, der
**                          WIndex ist dann die BuildProto-Nummer. Da
**                          f‰llt mir ein: Andreas' Patch hatte einen Bug
**                          provoziert, weil der Kraftwerk-WType verst¸mmelt
**                          wurde... (Probleme, wenn ein Kraftwerk gekillt
**                          werden sollen).
**      08-Apr-96   floh    die erste station‰re Bakterie einer Radarstation
**                          wird jetzt auf View = BuildProto->Power
**                          gepatcht.
**      17-May-96   floh    Randsektoren werden jetzt abgeblockt.
**      24-May-96   floh    Im Designer-Modus wird das Pixel in der
**                          Buildmap gesetzt.
**      09-Aug-96   floh    revised & updated (AFs letztes Update)
**      02-Nov-96   floh    beim Owner-Update wird SectorCount[]
**                          auf neuestem Stand gehalten
**      12-Apr-97   floh    + BP_Array nicht mehr global
**      26-Aug-97   floh    + setzt den Owner des Sektors nicht mehr
**                            direkt, sondern benutzt
**                            yw_energy.c/yw_SetOwner()
**      17-Sep-97   floh    + neues Arg, <no_bacts> gibt an, ob
**                            attachte Bakterien erzeugt werden sollen
**      16-Mar-98   floh    + Radarstations-Hack ist raus
*/
{
    struct Cell *sec = &(ywd->CellArea[sec_y * ywd->MapSizeX + sec_x]);
    struct BuildProto *bp = &(ywd->BP_Array[bp_num]);
    struct SectorDesc *sd = &(ywd->Sectors[bp->SecType]);
    ULONG k,l,lim;
    struct MinNode *nd;
    struct MinList *ls;
    Object *host_robo = NULL;
    struct VFMBitmap *blg_map, *typ_map;
    UBYTE *blg_body,*typ_body;
    BOOL   vehicle_allowed = TRUE;

    #ifdef __NETWORK__
    struct sendmessage_msg sm;
    struct ypamessage_buildingvehicle bv;
    memset( &bv, 0, sizeof( bv ) );
    #endif

    /*** Rand-Check ***/
    if ((sec_x == 0) || (sec_y == 0) ||
        (sec_x == (ywd->MapSizeX-1)) ||
        (sec_y == (ywd->MapSizeY-1)))
    {
        /*** ein RandSektor ***/
        return;
    };

    /*** Building-Map und TypeMap updaten ***/
    _get(ywd->BuildMap, BMA_Bitmap, &blg_map);
    blg_body = (UBYTE *) blg_map->Data;
    blg_body[sec_y*blg_map->Width + sec_x] = bp_num;
    _get(ywd->TypeMap, BMA_Bitmap, &typ_map);
    typ_body = (UBYTE *) typ_map->Data;
    typ_body[sec_y*typ_map->Width + sec_x] = bp->SecType;

    /*** Sektor initialisieren ***/
    sec->Type         = bp->SecType;
    sec->SType        = sd->SecType;
    sec->EnergyFactor = 0;
    sec->WType        = WTYPE_Building;
    sec->WIndex       = bp_num;

    /*** Anfangs-Energy initialisieren ***/
    if (sd->SecType == SECTYPE_COMPACT) {
        memset(sec->SubEnergy,0,sizeof(sec->SubEnergy));
        lim = 1;
    } else {
        lim = 3;
    };
    for (k=0; k<lim; k++) {
        for (l=0; l<lim; l++) {
            sec->SubEnergy[k][l] = sd->SSD[k][l]->init_energy;
        };
    };

    /*** Sonderbehandlung f¸r Kraftwerke       ***/
    /*** (WType wird dabei zu WTYPE_Kraftwerk) ***/
    if (bp->BaseType == BUILD_BASE_KRAFTWERK) {
        yw_AllocKraftWerk(ywd, sec_x, sec_y, bp->Power);
    };
    yw_SetOwner(ywd,sec,sec_x,sec_y,owner);

    /*** Falls station‰re Bakterien existieren, ***/
    /*** einen geeignenten Host-Robo suchen     ***/
    ls = &(ywd->CmdList);
    for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {

        struct OBNode *obnd = (struct OBNode *)nd;

        Object *r = obnd->o;
        struct Bacterium *rbact = obnd->bact;

        if ((BCLID_YPAROBO == rbact->BactClassID) &&
            (owner         == rbact->Owner))
        {
            /*** passenden Robo gefunden ***/
            host_robo = r;
            break;
        };
    };

    /* ---------------------------------------------------------------
    ** Netzwerk: Wenn wir ein richtiges netzwerkspiel am laufen
    ** haben und nicht der Userrobo sind, dann bauen wir keine statio-
    ** n‰ren vehicle
    ** -------------------------------------------------------------*/
    #ifdef __NETWORK__
    if( ywd->playing_network ) {
        if( host_robo != ywd->UserRobo )
            vehicle_allowed = FALSE;
        }
    #endif

    /*** station‰re Bakterien erzeugen (geht nur, wenn Hostrobo da) ***/
    if (!no_bacts) {
        if (host_robo && vehicle_allowed) {

            LONG   command_id;
            Object *first_vco = NULL;
            ULONG i=0;

            /*** F¸r das "Geschwader" eine ID holen ***/
            _get( host_robo, YRA_CommandCount, &command_id );
            command_id++;
            _set( host_robo, YRA_CommandCount, command_id );

            while ((bp->SBact[i].vp != 0) && (i<8)) {

                Object *vco;
                struct createvehicle_msg cv_msg;

                FLOAT mid_x,mid_z;

                /*** Sektor-Mittelpunkt ermitteln ***/
                mid_x = (sec_x * SECTOR_SIZE) + (SECTOR_SIZE/2);
                mid_z = -((sec_y * SECTOR_SIZE) + (SECTOR_SIZE/2));

                cv_msg.vp = bp->SBact[i].vp;
                cv_msg.x  = mid_x + bp->SBact[i].pos.x;
                cv_msg.y  = bp->SBact[i].pos.y;
                cv_msg.z  = mid_z + bp->SBact[i].pos.z;
                vco = (Object *) _methoda(world, YWM_CREATEVEHICLE, &cv_msg);

                /*** Bakterie initialisieren ***/
                if (vco) {

                    struct installgun_msg ig_msg;
                    struct setstate_msg ss_msg;
                    struct Bacterium *vcbact;

                    /*** Bacterium initialisieren ***/
                    _get(vco, YBA_Bacterium, &vcbact);
                    vcbact->Owner = owner;

                    /*** das Teil noch ausrichten ***/
                    ig_msg.flags   = 0;
                    ig_msg.basis.x = bp->SBact[i].vec.x;
                    ig_msg.basis.y = bp->SBact[i].vec.y;
                    ig_msg.basis.z = bp->SBact[i].vec.z;
                    _methoda(vco, YGM_INSTALLGUN, &ig_msg);

                    /*** GENESIS-Zustand aktivieren ***/
                    ss_msg.main_state = ACTION_CREATE;
                    ss_msg.extra_off  = 0;
                    ss_msg.extra_on   = 0;
                    _methoda(vco, YBM_SETSTATE_I, &ss_msg);
                    vcbact->scale_time = 500;
                    vcbact->scale_x    = 1.0;
                    vcbact->scale_y    = 1.0;
                    vcbact->scale_z    = 1.0;

                    vcbact->robo       = host_robo;
                    vcbact->CommandID  = command_id;

                    #ifdef __NETWORK__
                    if( ywd->playing_network ) {

                        vcbact->ident |= ( ((UBYTE)owner) << 24 );
                        bv.vehicle[ i ].ident   = vcbact->ident;
                        bv.vehicle[ i ].basis.x = bp->SBact[i].vec.x;
                        bv.vehicle[ i ].basis.y = bp->SBact[i].vec.y;
                        bv.vehicle[ i ].basis.z = bp->SBact[i].vec.z;
                        bv.vehicle[ i ].pos     = vcbact->pos;   // schon y-korr.
                        bv.vehicle[ i ].vproto  = bp->SBact[i].vp;
                        }
                    #endif

                    /*** in Kommando-Liste einklinken ***/
                    if (first_vco) _methoda(first_vco, YBM_ADDSLAVE, vco);
                    else {
                        first_vco = vco;
                        _methoda(host_robo, YBM_ADDSLAVE, vco);
                    };
                };

                /*** n‰chste station‰re Bakterie ***/
                i++;
            };
        };

        /* ------------------------------------------------------------------
        ** Es gibt 2 arten des bauens: Immediate beim Start und ¸ber Jobs
        ** im Spiel. Die Init-Sache macht jeder und der OrgRobo verschickt
        ** eine building-Vehicle-message. Der Sector selbst wird bei der
        ** Init-Sache von jedem selbst gesetzt und im game beim Start eines
        ** Bauauftrages. So haben wir zwar 2 Messages, aber eine einheitliche
        ** Bearbeitung f¸r beide Phasen! Yo, genau so!
        ** ----------------------------------------------------------------*/
        #ifdef __NETWORK__
        if( ywd->playing_network && vehicle_allowed && host_robo ) {

            bv.generic.message_id = YPAM_BUILDINGVEHICLE;
            bv.generic.timestamp  = ywd->TimeStamp;
            bv.generic.owner      = owner;

            sm.receiver_id         = NULL;
            sm.receiver_kind       = MSG_ALL;
            sm.sender_id           = ywd->gsr->NPlayerName;
            sm.sender_kind         = MSG_PLAYER;
            sm.data                = &bv;
            sm.data_size           = sizeof( bv );
            sm.guaranteed          = TRUE;
            _methoda( ywd->world, YWM_SENDMESSAGE, &sm );
            }
        #endif
    };

    /*** das war's ***/
}

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, yw_YWM_CREATEBUILDING, struct createbuilding_msg *msg)
/*
**  FUNCTION
**      Eine "Bauen-Anfrage". Ein Geb‰ude kann im
**      Immediate- oder im "Normal"-Modus gebaut werden.
**      Die Methode returniert FALSE, wenn der Bauauftrag
**      aus irgendeinem Grund abgewiesen wird.
**
**      ACHTUNG: Der Ziel-Sektor wird nicht auf G¸ltigkeit
**      getestet (also weder Energie- noch Eigent¸mer-Check).
**
**  INPUTS
**      msg.job_id      - welcher Job-Slot soll benutzt werden?
**                        -> siehe Diskussion in ypaworldclass.h
**                           unter YWM_CREATEBUILDING
**      msg.owner       - wird in den fertigen Sektor eingetragen
**      msg.bp          - Build-Prototype (siehe BuildProto-Array)
**      msg.immediate   - Geb‰ude wird sofort hingesetzt
**      msg.sec_x       - X-Koordinate des zu bebauenden Sektors
**      msg.sec_y       - ...
**      msg.flags       - CBF_NOBACTS
**
**  RESULTS
**      TRUE    - der Bauauftrag wurde akzeptiert, je nach
**                msg.immediate steht das Geb‰ude schon oder
**                wird innerhalb der n‰chsten ... Sekunden fertig sein
**      FALSE   - der entsprechende Job-Slot ist gerade belegt
**
**  CHANGED
**      20-Dec-95   floh    created
**      29-Jan-96   floh    fertig + debugging...
**      31-Jan-96   floh    Es wird zuerst untersucht, ob der Sektor
**                          irgendwie besetzt ist (gerade bebaut wird,
**                          etc). Falls es ein Kraftwerk ist,
**                          wird dieses zuerst logisch gekillt.
**      07-Feb-96   floh    + BUG: Ooops... das Semikolon...
**                          + alle Bakterien im Sektor (auﬂer
**                            Raketen und Robos) werden jetzt
**                            gekillt
**      27-Jun-96   floh    + experimentell: es werden gar keine
**                            Bakterien gekillt.
**      16-Dec-96   floh    + Bauen-Job wird abgewiesen, wenn
**                            User-Vehikel auf diesem Sektor.
**      09-Apr-97   floh    + Cancel-Tests im Immediate Modus jetzt
**                            etwas lascher (User-Vehicle-Test
**                            wird ignoriert.
**                          + falls sich auf dem zu bebauenden Sektor
**                            ein Robo befindet, wird der Job gecancelt.
**      02-May-97   floh    + im Player gab es einen Abgang, weil
**                            UVBact nicht initialisiert ist
**      17-Sep-97   floh    + <flags> Feld in Message
*/
{
    struct ypaworld_data *ywd = INST_DATA(cl,o);
    BOOL  vinsec = FALSE;
    ULONG secx = msg->sec_x;
    ULONG secy = msg->sec_y;
    struct Bacterium *bact;

    struct Cell *sec = &(ywd->CellArea[secy * ywd->MapSizeX + secx]);

    /*** ein Viewer-Vehikel oder ein Robo im Sektor? ***/
    bact = (struct Bacterium *) sec->BactList.mlh_Head;
    while( bact->SectorNode.mln_Succ ) {
        if (bact == ywd->UVBact) {    
            vinsec = TRUE;
            break;
        };
        if (BCLID_YPAROBO == bact->BactClassID) {
            vinsec = TRUE;
            break;
        };
        bact = (struct Bacterium *) bact->SectorNode.mln_Succ;
    };
    if (ywd->UVBact) {
        if (ywd->UVBact->Sector == sec) vinsec=TRUE;
    };

    /*** zuerst gucken, ob der Sektor irgendeine Extrawurst ist ***/
    if (WTYPE_JobLocked == sec->WType) {

        /*** wird gerade bebaut -> abweisen ***/
        return(FALSE);

    } else if ((vinsec) && (!msg->immediate)) {

        /*** User-Vehikel ist auf Sektor -> abweisen ***/
        return(FALSE);

    } else if (WTYPE_Kraftwerk == sec->WType) {

        /*** war vorher ein Kraftwerk -> logisch abmelden ***/
        yw_FreeKraftWerk(ywd, sec->WIndex);

    } else if (((WTYPE_Wunderstein == sec->WType) ||
               (WTYPE_ClosedGate == sec->WType)   ||
               (WTYPE_OpenedGate == sec->WType))  &&
               (!msg->immediate))
    {
        /*** irgendein "nicht bebaubarer" Sektor, abweisen ***/
        return(FALSE);
    } else if ((WTYPE_DeadWunderstein==sec->WType) && (ywd->playing_network)) {
        /*** im Netzwerk-Spiel auch eroberten WStein nicht bebauen! ***/
        return(FALSE);
    };

    /*** im Normal-Modus, Sektor per BuildJob hochskalieren ***/
    if (msg->immediate) {
        yw_SetSector(o,ywd,secx,secy,msg->owner,msg->bp,(msg->flags & CBF_NOBACTS));
    } else {
        /*** Fahrzeuge zerstoeren ***/
        yw_BactOverkill(ywd,secx,secy);
 
        /*** sonst einen TimedLock beantragen ***/
        if (!yw_LockBuildJob(ywd,msg->job_id,3000,secx,secy,msg->owner,msg->bp))
            return(FALSE);
    };

    /*** Ende ***/
    return(TRUE);
}

/*=================================================================**
**                                                                 **
**  BUILD PROTOTYPE SCRIPT PARSER                                  **
**                                                                 **
**=================================================================*/

/*-----------------------------------------------------------------*/
ULONG yw_BuildProtoParser(struct ScriptParser *p)
/*
**  FUNCTION
**      Parser f¸r BuildProto Descriptions
**      (new_building, modifiy_building Blˆcke)
**
**  CHANGED
**      03-May-96   floh    created
**      08-May-96   floh    Fix f¸r Loop-Optimizer-Bug im Watcom (FUCK!)
**      21-Sep-96   floh    Designer: alle Buildings f¸r User enabled
**      12-Apr-97   floh    + BP_Array nicht mehr global
*/
{
    UBYTE *kw   = p->keyword;
    UBYTE *data = p->data;
    struct ypaworld_data *ywd = (struct ypaworld_data *) p->store[1];

    if (PARSESTAT_READY == p->status) {

        /*** momentan auﬂerhalb eines Contexts ***/
        struct BuildProto *bp = NULL;
        p->target = NULL;

        /*** NEW ***/
        if (stricmp(kw, "new_building") == 0) {

            /*** neuer VehicleProto ***/
            ULONG bp_num = strtol(data, NULL, 0);
            if (bp_num >= NUM_BUILDPROTOS) return(PARSE_BOGUS_DATA);
            else {

                bp = &(ywd->BP_Array[bp_num]);
                memset(bp, 0, sizeof(struct BuildProto));

                /*** Attribute auf Default ***/
                bp->CEnergy  = 50000;
                bp->TypeIcon = 'A';
                bp->Noise[BP_NOISE_WORKING].volume = 120;
            };

        /*** MODIFY ***/
        } else if (stricmp(kw, "modify_building") == 0) {

            /*** existierenden BuildProto modifizieren ***/
            ULONG bp_num = strtol(data, NULL, 0);
            if (bp_num >= NUM_BUILDPROTOS) return(PARSE_BOGUS_DATA);
            else {
                bp = &(ywd->BP_Array[bp_num]);
                ywd->touch_stone.bp_num = bp_num;
            };

        } else return(PARSE_UNKNOWN_KEYWORD);

        /*** Script-Parser-Struktur f¸r weitere Aufrufe initialisieren ***/
        p->target  = bp;
        p->status  = PARSESTAT_RUNNING;
        p->store[0] = 0;    // act_bact
        return(PARSE_ENTERED_CONTEXT);

    } else {

        /*** innerhalb eines Context-Block ***/
        struct BuildProto *bp = p->target;
        ULONG act_bact = p->store[0];

        /*** END ***/
        if (stricmp(kw, "end") == 0){

            #ifdef YPA_DESIGNMODE
            /*** f¸r User enablen ***/
            bp->FootPrint |= (1<<1);
            #endif

            p->status = PARSESTAT_READY;
            p->target = NULL;
            return(PARSE_LEFT_CONTEXT);

        /*** model ***/
        }else if (stricmp(kw, "model") == 0){

            if (stricmp(data, "building") == 0)
                bp->BaseType = BUILD_BASE_NONE;
            else if (stricmp(data, "kraftwerk") == 0)
                bp->BaseType = BUILD_BASE_KRAFTWERK;
            else if (stricmp(data, "radar") == 0)
                bp->BaseType = BUILD_BASE_RADARSTATION;
            else if (stricmp(data, "defcenter") == 0)
                bp->BaseType = BUILD_BASE_DEFCENTER;
            else return(PARSE_BOGUS_DATA);

        /*** ENABLE ***/
        }else if (stricmp(kw, "enable") == 0){
            ULONG own = strtol(data, NULL, 0);
            bp->FootPrint |= (1<<own);

        /*** DISABLE ***/
        }else if (stricmp(kw, "disable") == 0){
            ULONG own = strtol(data, NULL, 0);
            bp->FootPrint &= ~(1<<own);

        /*** misc ***/
        }else if (stricmp(kw, "name") == 0){

            /*** Name kopieren und '_' ersetzen durch ' ' ***/
            UBYTE *s;
            if (strlen(data) >= (sizeof(bp->Name)-1)) {
                _LogMsg("BuildProtoParser(): Name too long!\n");
                return(PARSE_BOGUS_DATA);
            };
            strcpy(bp->Name, data);
            s = bp->Name;
            while (s = strchr(s,'_')) *s++ = ' ';

        }else if (stricmp(kw, "power") == 0)
            bp->Power = strtol(data, NULL, 0);
        else if (stricmp(kw, "energy") == 0)
            bp->CEnergy = strtol(data, NULL, 0);

        /*** Visuals ***/
        else if (stricmp(kw, "sec_type") == 0)
            bp->SecType = strtol(data, NULL, 0);
        else if (stricmp(kw, "type_icon") == 0)
            bp->TypeIcon = data[0];

        /*** Sound Samples ***/
        else if (stricmp(kw, "snd_normal_sample") == 0) {
            if (strlen(data) >= (sizeof(bp->Noise[0].smp_name)-1)) {
                _LogMsg("BuildProtoParser(): Sample name too long!\n");
                return(PARSE_BOGUS_DATA);
            };
            strcpy(bp->Noise[BP_NOISE_WORKING].smp_name, data);

        /*** Sound Volume ***/
        }else if (stricmp(kw, "snd_normal_volume") == 0)
            bp->Noise[BP_NOISE_WORKING].volume = strtol(data, NULL, 0);

        /*** Sound Pitches ***/
        else if (stricmp(kw, "snd_normal_pitch") == 0)
            bp->Noise[BP_NOISE_WORKING].pitch = strtol(data, NULL, 0);

        /*** station‰re Bakterien ***/
        else if (stricmp(kw, "sbact_act") == 0)
            act_bact = strtol(data, NULL, 0);
        else if (stricmp(kw, "sbact_vehicle") == 0)
            bp->SBact[act_bact].vp = strtol(data, NULL, 0);
        else if (stricmp(kw, "sbact_pos_x") == 0)
            bp->SBact[act_bact].pos.x = atof(data);
        else if (stricmp(kw, "sbact_pos_y") == 0)
            bp->SBact[act_bact].pos.y = atof(data);
        else if (stricmp(kw, "sbact_pos_z") == 0)
            bp->SBact[act_bact].pos.z = atof(data);
        else if (stricmp(kw, "sbact_dir_x") == 0)
            bp->SBact[act_bact].vec.x = atof(data);
        else if (stricmp(kw, "sbact_dir_y") == 0)
            bp->SBact[act_bact].vec.y = atof(data);
        else if (stricmp(kw, "sbact_dir_z") == 0)
            bp->SBact[act_bact].vec.z = atof(data);

        /*** Unknown Keyword ***/
        else return(PARSE_UNKNOWN_KEYWORD);

        /*** Parse-Data updaten f¸r n‰chsten Aufruf ***/
        p->store[0] = act_bact;
        return(PARSE_ALL_OK);
    };

    /*** can't happen ***/
    return(PARSE_UNKNOWN_KEYWORD);
}


