/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_energy.c,v $
**  $Revision: 38.20 $
**  $Date: 1997/03/22 18:48:38 $
**  $Locker: floh $
**  $Author: floh $
**
**  diverses netzgerümpel
**
**  (C) Copyright 1997 by A.Flemming
*/
#ifdef _MSC_VER
#include <windows.h>
#endif
#include <exec/types.h>
#include <exec/memory.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#ifdef _MSC_VER
#include "nucleus/io.h"
#endif
#include "ypa/ypaworldclass.h"
#include "ypa/ypabactclass.h"
#include "ypa/ypagunclass.h"
#include "ypa/ypamissileclass.h"

#include "yw_protos.h"
#include "yw_gsprotos.h"
#include "yw_netprotos.h"
#include "thirdparty/eulerangles.h"

_extern_use_nucleus
extern UBYTE  **GlobalLocaleHandle;

char NETLOGNAME[ 300 ];
int  NETLOGCOUNT = 0;
extern struct ConfigItem yw_ConfigItems[];

/*** Global, um die nicht auf dem Stack zu haben ***/
struct ypamessage_update upd;


/*-----------------------------------------------------------------*/
void yw_GetNetGemProtos( struct ypaworld_data *ywd, struct Wunderstein *gem,
                         WORD *vproto, WORD *bproto )
{

    switch( ywd->gsr->NPlayerOwner ) {

        case 1:  *vproto = gem->nw_vproto_num[ 0 ]; break;
        case 6:  *vproto = gem->nw_vproto_num[ 1 ]; break;
        case 3:  *vproto = gem->nw_vproto_num[ 2 ]; break;
        case 4:  *vproto = gem->nw_vproto_num[ 3 ]; break;
        default: *vproto = 0;
        }
    

    switch( ywd->gsr->NPlayerOwner ) {

        case 1:  *bproto = gem->nw_bproto_num[ 0 ]; break;
        case 6:  *bproto = gem->nw_bproto_num[ 1 ]; break;
        case 3:  *bproto = gem->nw_bproto_num[ 2 ]; break;
        case 4:  *bproto = gem->nw_bproto_num[ 3 ]; break;
        default: *bproto = 0;
        }
}


void yw_ActivateNetWunderstein(struct ypaworld_data *ywd,
                               struct Cell *sec,
                               ULONG which_gem)
/*
**  FUNCTION
**      Aktiviert einen "eroberten" Wunderstein, indem
**      das Prototype-Script geparst wird und der
**      Logmsg-File angezeigt (nyi).
**
**  INPUTS
**      ywd     - Ptr auf LID des Welt-Objects
**      sec     - Sektor, in dem der Wunderstein entdeckt wurde
**      which   - Nummer des Wundersteins (0..7)
**
**  RESULTS
**      Es wird eine Fehlermeldung ausgegeben, wenn das
**      Parsen schief ging, das nützt aber nix, weil das
**      mitten im Spiel passiert!
**
**  CHANGED
**      27-Sep-97   floh    + Wunderstein-Logmsg in Ordnung gebracht.
**      19-Feb-98   floh    + neue Wunderstein-Anzeige (per Touchstone
**                            Struktur) in Ordnung gebracht
*/
{
    struct Wunderstein *gem = &(ywd->gem[which_gem]);
    struct logmsg_msg lm;
    UBYTE  msg[128];
    WORD   vproto, bproto;

    /*** Prototypen ermitteln und TouchStone Struktur ausfüllen ***/
    yw_GetNetGemProtos( ywd, gem, &vproto, &bproto );
    ywd->touch_stone.gem = which_gem;
    ywd->touch_stone.time_stamp = ywd->TimeStamp;
    ywd->touch_stone.vp_num = vproto;
    ywd->touch_stone.bp_num = bproto;
    ywd->touch_stone.wp_num = 0;

    /*** Vehicle oder Building freischalten ***/
    if( vproto ) {

        /*** Vehicle freischalten ***/
        ywd->VP_Array[ vproto ].FootPrint  = 0;
        ywd->VP_Array[ vproto ].FootPrint |= (1 << ywd->gsr->NPlayerOwner);
        }

    if( bproto ) {

        /*** Gebäude freischalten ***/
        ywd->BP_Array[ bproto ].FootPrint  = 0;
        ywd->BP_Array[ bproto ].FootPrint |= (1 << ywd->gsr->NPlayerOwner);
        }

    /*** Erklärungs-Message anzeigen ***/
    strcpy(msg,ypa_GetStr(ywd->LocHandle,STR_LMSG_WUNDERSTEIN,"TECHNOLOGY UPGRADE!\n"));
    lm.bact = NULL;
    lm.pri  = 48;
    lm.msg  = msg;
    if (gem->type != 0) lm.code = gem->type;
    else                lm.code = 0;
    _methoda(ywd->world,YWM_LOGMSG,&lm);

    /*** Message verschicken ***/
    #ifdef __NETWORK__
    if( (ywd->playing_network) && (ywd->exclusivegem) ) {

        struct sendmessage_msg sm;
        struct ypamessage_gem  gm;

        gm.generic.message_id = YPAM_GEM;
        gm.generic.owner      = ywd->gsr->NPlayerOwner;
        gm.gemoffset          = which_gem;
        gm.enable             = 1;          // bei anderen wieder einschalten

        sm.receiver_id        = NULL;
        sm.receiver_kind      = MSG_ALL;
        sm.data               = &gm;
        sm.data_size          = sizeof( gm );
        sm.guaranteed         = TRUE;
        _methoda( ywd->world, YWM_SENDMESSAGE, &sm );
        }
    #endif

    /*** Wunderstein deaktivieren, kann per Message wieder aktiv werden ***/
    sec->WType  = WTYPE_DeadWunderstein;
}


void yw_DisableNetWunderstein(struct ypaworld_data *ywd,
                              struct Cell *sec,
                              ULONG which_gem)
/*
**  FUNCTION
**      Schaltet einen Wunderstein aus. Für den Rest der Welt können wir
**      die Standardmessage nehmen, wir müssen lediglich das enable-Flag
**      ausschalten, weil bei anderen der Wunderstein nicht wieder ein-
**      geschalten werden darf.
**      Bei mir wird ausgeschalten!
**
**  INPUTS
**      ywd     - Ptr auf LID des Welt-Objects
**      sec     - Sektor, in dem der Wunderstein entdeckt wurde
**      which   - Nummer des Wundersteins (0..7)
**
**  RESULTS
**
**  CHANGED
**      27-Sep-97   floh    LOGMSG_TECH_LOST mit VoiceOver Code und Pri.
*/
{
    struct Wunderstein *gem = &(ywd->gem[which_gem]);
    struct logmsg_msg lm;
    UBYTE  msg[128];
    WORD   vproto, bproto;

    /*** Prototypen ermitteln ***/
    yw_GetNetGemProtos( ywd, gem, &vproto, &bproto );

    /*** Vehicle oder Building freischalten ***/
    if( vproto )
        ywd->VP_Array[ vproto ].FootPrint  = 0;

    if( bproto )
        ywd->BP_Array[ bproto ].FootPrint  = 0;

    /*** Erklärungs-Message anzeigen ***/
    strcpy(msg,ypa_GetStr(ywd->LocHandle,STR_LMSG_TECH_LOST,"TECH-UPGRADE LOST!  "));
    strcat(msg,gem->msg);
    lm.bact = NULL;
    lm.pri  = 80;
    lm.msg  = msg;
    lm.code = LOGMSG_TECH_LOST;
    _methoda(ywd->world,YWM_LOGMSG,&lm);

    /*** Message verschicken ***/
    #ifdef __NETWORK__
    if( ywd->playing_network ) {

        struct sendmessage_msg sm;
        struct ypamessage_gem  gm;

        gm.generic.message_id = YPAM_GEM;
        gm.generic.owner      = ywd->gsr->NPlayerOwner;
        gm.gemoffset          = which_gem;
        gm.enable             = 0;          // bei anderen nicht einschalten

        sm.receiver_id        = NULL;
        sm.receiver_kind      = MSG_ALL;
        sm.data               = &gm;
        sm.data_size          = sizeof( gm );
        sm.guaranteed         = TRUE;
        _methoda( ywd->world, YWM_SENDMESSAGE, &sm );
        }
    #endif

    /*** Wunderstein deaktivieren ***/
    sec->WType  = 0;
    sec->WIndex = 0;
}


_dispatcher( ULONG, yw_YWM_SENDMESSAGE, struct sendmessage_msg *sm )
{
/*
**  FUNCTION:   Dies ist ein FrontEnd für das Senden von Messages.
**              Es werden viele spielspezifische und debug-daten
**              in die Message geschrieben, die häufig gleich
**              sind bzw. der Übersichtlichkeit wegen an zentraler
**              Stelle verwaltet werden sollten.
**
**
*/

    struct ypaworld_data *ywd = INST_DATA( cl, o );
    
    if( ywd->gsr->dont_send ) return( 0L );

    /*** lokale Abschußzeit ***/
    ((struct ypamessage_generic *)(sm->data))->timestamp    = ywd->TimeStamp;

    /* -------------------------------------------------------------------
    ** Zähler, wenn Eigentümer. Den VehicleenergyTest machen wir, weil das
    ** der Eigentümer sich auf den Empfänger, und nicht auf den Sender be-
    ** zieht. Außerdem achte ich darauf, daß es Messages an alle sind,
    ** weil sonst unterschiedliche Empfänger unterschiedliche Zähler für
    ** den jeweiligen Eigentümer haben.
    ** -----------------------------------------------------------------*/
    if( ( ywd->gsr->NPlayerOwner) &&
        ( sm->receiver_kind  == MSG_ALL ) &&
        ( YPAM_VEHICLEENERGY != ((struct ypamessage_generic *)(sm->data))->message_id ) ) {

        ((struct ypamessage_generic *)(sm->data))->msgcount = ywd->gsr->msgcount++;
        }

    /*** Senderdaten ***/
    sm->sender_id   = ywd->gsr->NPlayerName;
    sm->sender_kind = MSG_PLAYER;

    /*** Und los ***/
    return( _methoda( ywd->nwo, NWM_SENDMESSAGE, sm ) );
}



void yw_AddVehicleToUpdate( struct vhclupd_entry *entry,
                            ULONG  what,
                            struct Bacterium *bact )
{
    /* -------------------------------------------------------------
    ** Fügt alle Daten zum übergebenen Vehicle ein
    ** Dazu braucht er:
    **      Alle Vehicle und Waffen.
    **      Deren Zustände (MainState & ExtraState reicht, Visproto-
    **                      abweichungen ignorieren wir)
    **      Position und Speed
    **      ident
    ** -----------------------------------------------------------*/
    EulerAngles ea;
    HMatrix hm;

    entry->kind       = what;
    entry->pos        = bact->pos;
    entry->mainstate  = bact->MainState;
    entry->extrastate = bact->ExtraState;
    entry->ident      = bact->ident;
    entry->vproto     = bact->TypeID;
    entry->energy     = bact->Energy;

    /*** Matrix ist etwas mehr Aufwand ***/
    hm[0][0]=bact->dir.m11; hm[0][1]=bact->dir.m12; hm[0][2]=bact->dir.m13; hm[0][3]=0.0;
    hm[1][0]=bact->dir.m21; hm[1][1]=bact->dir.m22; hm[1][2]=bact->dir.m23; hm[1][3]=0.0;
    hm[2][0]=bact->dir.m31; hm[2][1]=bact->dir.m32; hm[2][2]=bact->dir.m33; hm[2][3]=0.0;
    hm[3][0]=0.0;           hm[3][1]=0.0;           hm[3][2]=0.0;           hm[3][3]=1.0;
    ea = Eul_FromHMatrix(hm,EulOrdXYZs);
    entry->roll  = (UBYTE) FLOAT_TO_INT(((ea.x/(2*PI))*127.0));
    entry->pitch = (UBYTE) FLOAT_TO_INT(((ea.y/(2*PI))*127.0));
    entry->yaw   = (UBYTE) FLOAT_TO_INT(((ea.z/(2*PI))*127.0));

}


void yw_SendGameCopy( struct ypaworld_data *ywd, UBYTE owner, UBYTE *receiver_id )
{
    /* -------------------------------------------------------------
    ** Verschickt eine komplette Kopie meiner Vehicle, so daß der,
    ** der rumgemeckert hat, daraus ein neues abbild erstellen kann.
    ** Achtung! Wegen den raketen, die an Roboflaks hängen können,
    ** übergeben wir diese ganz normal mit. Wenn auf der anderen
    ** Seite dann aber der Robo erzeugt wird, dürfen wir nicht neu
    ** erzeugen, sondern nur ausfüllen! Deshalb werden diese extra
    ** gekennzeichnet.
    ** Die updatemessage muss global sein (Stack, denn das Ding ist
    ** 32k gross)
    ** -----------------------------------------------------------*/
    struct sendmessage_msg sm;
    struct OBNode *robo, *commander, *slave, *waffe;
    struct vhclupd_entry *eintrag;
    ULONG  count, max_items;
    
    /*** weil es noch eines Endeeintrags bedarf ***/
    max_items = MAXNUM_UPDATEITEMS - 1;

    /*** Daten korrect? ***/
    if( owner == 0 ) {

        yw_NetLog("\n+++ UPD: Owner 0 requested update???\n");
        return;
        }

    if( receiver_id == NULL ) {

        yw_NetLog("\n+++ UPD: No receiver given for Update\n");
        return;
        }

    /*** Meinen Robo suchen ***/
    if( !( robo = yw_GetRoboByOwner( ywd, owner ) )) {

        yw_NetLog("\n+++ UPD:No Robo found for owner %d/receiver %s\n", owner, receiver_id);
        return;
        }

    count = 0;

    /*** Robo eintragen ***/
    eintrag = &( upd.data[ count++ ] );
    yw_AddVehicleToUpdate( eintrag, UPD_ROBO, robo->bact );

    /*** Waffen eintragen ***/
    waffe = (struct OBNode *) robo->bact->auto_list.mlh_Head;
    while( waffe->nd.mln_Succ ) {

        eintrag = &( upd.data[ count++ ]);
        yw_AddVehicleToUpdate( eintrag, UPD_MISSILE, waffe->bact );
        if( count >= max_items ) break;
        waffe = (struct OBNode *) waffe->nd.mln_Succ;
        }

    commander = (struct OBNode *) robo->bact->slave_list.mlh_Head;
    while( commander->nd.mln_Succ ) {

        ULONG rgun = 0L;

        /*** Commander (test, ob Robogun) ***/
        eintrag = &( upd.data[ count++ ]);
        if( BCLID_YPAGUN == commander->bact->BactClassID )
            _get( commander->o, YGA_RoboGun, &rgun );

        if( rgun )
            yw_AddVehicleToUpdate( eintrag, UPD_RGUN, commander->bact );
        else
            yw_AddVehicleToUpdate( eintrag, UPD_CMDR, commander->bact );
        if( count >= max_items ) break;

        /*** Seine Waffen ***/
        waffe = (struct OBNode *) commander->bact->auto_list.mlh_Head;
        while( waffe->nd.mln_Succ ) {

            eintrag = &( upd.data[ count++ ]);
            yw_AddVehicleToUpdate( eintrag, UPD_MISSILE, waffe->bact );
            if( count >= max_items ) break;
            waffe = (struct OBNode *) waffe->nd.mln_Succ;
            }

        /*** Slaves ***/
        slave = (struct OBNode *) commander->bact->slave_list.mlh_Head;
        while( slave->nd.mln_Succ ) {

            eintrag = &( upd.data[ count++ ]);
            yw_AddVehicleToUpdate( eintrag, UPD_SLAVE, slave->bact );
            if( count >= max_items ) break;

            /*** Seine Waffen ***/
            waffe = (struct OBNode *) slave->bact->auto_list.mlh_Head;
            while( waffe->nd.mln_Succ ) {

                eintrag = &( upd.data[ count++ ]);
                yw_AddVehicleToUpdate( eintrag, UPD_MISSILE, waffe->bact );
                if( count >= max_items ) break;
                waffe = (struct OBNode *) waffe->nd.mln_Succ;
                }

            slave = (struct OBNode *) slave->nd.mln_Succ;
            }

        commander = (struct OBNode *) commander->nd.mln_Succ;
        }

    /*** Endemarkierung ***/
    eintrag = &( upd.data[ count++ ]);
    eintrag->kind = UPD_END;

    /*** Verschicken ***/
    upd.generic.owner      = owner;
    upd.generic.message_id = YPAM_UPDATE;

    upd.basic.size         = sizeof( struct ypamessage_generic ) +
                             sizeof( struct ypamessage_update_basic) +
                             sizeof( struct vhclupd_entry ) * count;
    upd.basic.entries      = count;

    sm.receiver_id         = receiver_id;
    sm.receiver_kind       = MSG_PLAYER;
    sm.data                = &upd;
    sm.data_size           = upd.basic.size;
    sm.guaranteed          = TRUE;
    if( !_methoda( ywd->world, YWM_SENDMESSAGE, &sm ))
        yw_NetLog("\n+++ UPD: Send-Error. Hmmmm...\n");
    else
        yw_NetLog("\n+++ UPD: Ok, sent Update from owner %d to reciever %s\n", owner, receiver_id);
}



void yw_InsertUpdateData( struct vhclupd_entry *entry, struct Bacterium *bact,
                          UBYTE owner, Object *robo )
{
    /* ----------------------------------------------------------------
    ** Füllt bacterienstruktur aus und setzt den aktuellen Status, auch
    ** wenn Visproto und State(s) evtl. voneinander abweichen
    ** --------------------------------------------------------------*/
    struct setstate_msg state;
    EulerAngles ea;
    HMatrix     hm;

    bact->Owner = owner;

    /*** Matrix wieder ermitteln ***/
    ea.x = (((float)(entry->roll))/127.0)*2*PI;
    ea.y = (((float)(entry->pitch))/127.0)*2*PI;
    ea.z = (((float)(entry->yaw))/127.0)*2*PI;
    ea.w = EulOrdXYZs;
    Eul_ToHMatrix(ea,hm);
    bact->dir.m11=hm[0][0]; bact->dir.m12=hm[0][1]; bact->dir.m13=hm[0][2];
    bact->dir.m21=hm[1][0]; bact->dir.m22=hm[1][1]; bact->dir.m23=hm[1][2];
    bact->dir.m31=hm[2][0]; bact->dir.m32=hm[2][1]; bact->dir.m33=hm[2][2];

    if( robo )
        bact->robo = robo;
    else
        bact->robo = NULL;

    bact->ident  = entry->ident;
    bact->Energy = entry->energy;

    /*** Status setzen ***/
    bact->ExtraState = entry->extrastate;
    bact->MainState  = entry->mainstate;
    if( bact->ExtraState & EXTRA_MEGADETH ) {
        state.extra_on  = EXTRA_MEGADETH;
        state.extra_off = state.main_state = 0;
        }
    else {

        if( bact->ExtraState & EXTRA_FIRE ) {
            state.extra_on  = EXTRA_FIRE;
            state.extra_off = state.main_state = 0;
            }
        else {
            state.extra_off  = state.extra_on = 0L;
            state.main_state = entry->mainstate;
            }
        }

    _methoda( bact->BactObject, YBM_SETSTATE_I, &state );
}



BOOL yw_RestoreShadows( struct ypaworld_data *ywd,
                        struct ypamessage_update *upd,
                        UBYTE  owner )
{
    /*
    **  FUNCTION
    **
    **      Für den Owner "owner" kommen neue Daten. Wir machen folgendes:
    **      Zuerst suchen wir den zugehörigen Robo.
    **
    **      Wichtig:
    **       o Nur Geschwaderstruktur killen, nicht den Spieler löschen!
    **       o Beim Aufräumen von Angreifern abmelden
    **       o Raketen beachten!
    **       o Beim erzeugen darauf achten, daß Roboguns mit geliefert
    **         werden, aber nicht erzeugt, sondern nur ausgefüllt werden dürfen
    **
    **  RESULT
    **      TRUE, wenn alles ok war, FALSE kann bedeuten, daß das Spiel
    **      abgebrochen werden muß (wenn ich die Schatten gekillt habe, aber
    **      keine neuen machen konnte)
    **
    */

    struct  Node *node;
    Object *ActualRobo, *ActualFlak, *ActualCmdr, *ActualSlave, *ActualWeapon;
    struct  Bacterium *FlakBact, *RoboBact, *CmdrBact, *SlaveBact, *RifleMan;
    struct  Bacterium *WeaponBact;
    struct  OBNode *robo, *FlakNode;
    ULONG   count;
    struct  createvehicle_msg cv;
    struct  createweapon_msg cw;

    /*** Robo ermitteln ***/
    if( !( robo = yw_GetRoboByOwner( ywd, owner ) )) return( FALSE );

    /*** Schatten freigeben ***/
    yw_RemoveAllShadows( ywd, robo );

    /*** Nun wieder neu aufbauen ***/
    count = 0;
    while( upd->data[ count ].kind != UPD_END ) {

        struct vhclupd_entry *entry;

        entry = &( upd->data[ count++ ] );

        switch( entry->kind ) {

            case UPD_ROBO:

                /* ----------------------------------------------------
                ** Robo erzeugen, merken und Guns für das überschreiben
                ** vorbereiten (also erste Gun merken, kann dann nach
                ** jeder Robogun hochgezählt werden
                ** --------------------------------------------------*/

                cv.x    = entry->pos.x;
                cv.y    = entry->pos.y;
                cv.z    = entry->pos.z;
                cv.vp   = entry->vproto;
                ActualRobo = (Object *)_methoda( ywd->world, YWM_CREATEVEHICLE, &cv );
                if( !ActualRobo ) {
                    yw_NetLog("RESTORE: Unable to create robo\n");
                    return( FALSE );
                    }

                /*** Commanderliste besteht derzeit nur aus Flaks ***/
                _get( ActualRobo, YBA_Bacterium, &RoboBact );
                if( RoboBact->slave_list.mlh_Head->mln_Succ ) {
                    ActualFlak = ((struct OBNode *)RoboBact->slave_list.mlh_Head)->o;
                    FlakBact   = ((struct OBNode *)RoboBact->slave_list.mlh_Head)->bact;
                    FlakNode   =  (struct OBNode *)RoboBact->slave_list.mlh_Head;
                    }
                else {
                    FlakBact   = NULL;
                    FlakNode   = NULL;
                    ActualFlak = NULL;
                    }

                /*** Ausfüllen ***/
                yw_InsertUpdateData( entry, RoboBact, owner, NULL );

                /*** Einklinken ***/
                _methoda( ywd->world, YWM_ADDCOMMANDER, ActualRobo );

                /*** Wer bekommt Waffen? ***/
                RifleMan = RoboBact;
                break;

            case UPD_RGUN:

                /* ---------------------------------------------------
                ** Dös is ne Flak, die allerdings schon existiert. Wir
                ** müssen diese nur noch ausfüllen
                ** -------------------------------------------------*/

                if( !ActualFlak ) break;

                /*** Ausfüllen ***/
                yw_InsertUpdateData( entry, FlakBact, owner, ActualRobo );

                /* -------------------------------------------------------------
                ** nächste holen. Es können aber auch mittlerweile andere in der
                ** Liste sein. Die Flaks sind aber alle nach dieser ersten
                ** Flak.
                ** -----------------------------------------------------------*/
                while( FlakNode->nd.mln_Succ ) {

                    FlakNode   = (struct OBNode *) FlakNode->nd.mln_Succ;
                    if( BCLID_YPAGUN == FlakNode->bact->BactClassID ) {

                        ULONG rgun = 0L;
                        _get( FlakNode->o, YGA_RoboGun, &rgun );
                        if( rgun ) {

                            ActualFlak = FlakNode->o;
                            FlakBact   = FlakNode->bact;

                            /*** Wer bekommt Waffen? ***/
                            RifleMan   = FlakBact;
                            break;
                            }
                        }

                    ActualFlak = NULL;
                    }

                if( !ActualFlak ) {
                    FlakBact   = NULL;
                    FlakNode   = NULL;
                    ActualFlak = NULL;
                    }
                break;

            case UPD_CMDR:

                /* -------------------------------------------------
                ** Wie beim Robo, Erzeugen, ausfüllen und einklinken
                ** -----------------------------------------------*/
                cv.x       = entry->pos.x;
                cv.y       = entry->pos.y;
                cv.z       = entry->pos.z;
                cv.vp      = entry->vproto;
                ActualCmdr = (Object *)_methoda( ywd->world, YWM_CREATEVEHICLE, &cv );
                if( !ActualCmdr ) {
                    yw_NetLog("RESTORE: Unable to create cmdr\n");
                    return( FALSE );
                    }

                /*** Bact holen ***/
                _get( ActualCmdr, YBA_Bacterium, &CmdrBact );

                /*** Ausfüllen ***/
                yw_InsertUpdateData( entry, CmdrBact, owner, ActualRobo );

                /*** Einklinken ***/
                _methoda( ActualRobo, YBM_ADDSLAVE, ActualCmdr );

                /*** Wer bekommt Waffen? ***/
                RifleMan = CmdrBact;
                break;

            case UPD_SLAVE:

                /* -------------------------------------------------
                ** Wie beim Robo, Erzeugen, ausfüllen und einklinken
                ** -----------------------------------------------*/
                cv.x    = entry->pos.x;
                cv.y    = entry->pos.y;
                cv.z    = entry->pos.z;
                cv.vp   = entry->vproto;
                ActualSlave = (Object *)_methoda( ywd->world, YWM_CREATEVEHICLE, &cv );
                if( !ActualSlave ) {
                    yw_NetLog("RESTORE: Unable to create slave\n");
                    return( FALSE );
                    }

                /*** Bact holen ***/
                _get( ActualSlave, YBA_Bacterium, &SlaveBact );

                /*** Ausfüllen ***/
                yw_InsertUpdateData( entry, SlaveBact, owner, ActualRobo );

                /*** Einklinken ***/
                _methoda( ActualCmdr, YBM_ADDSLAVE, ActualSlave );

                /*** Wer bekommt Waffen? ***/
                RifleMan = SlaveBact;
                break;

            case UPD_MISSILE:

                /* -------------------------------------------------
                ** Waffe erzeugen und dem jeweiligen Wirt übergeben.
                ** Beim Einklinken e bissel aufpassen, ne?
                ** -----------------------------------------------*/
                cw.x    = entry->pos.x;
                cw.y    = entry->pos.y;
                cw.z    = entry->pos.z;
                cw.wp   = entry->vproto;
                ActualWeapon = (Object *)_methoda( ywd->world, YWM_CREATEWEAPON, &cw );
                if( !ActualWeapon ) {
                    yw_NetLog("RESTORE: Unable to create weapon\n");
                    return( FALSE );
                    }

                /*** Bact holen ***/
                _get( ActualWeapon, YBA_Bacterium, &WeaponBact );

                /*** Ausfüllen ***/
                yw_InsertUpdateData( entry, WeaponBact, owner, ActualRobo );

                /*** Einklinken ***/
                if( WeaponBact->master ) {

                    _Remove( (struct Node *) &(WeaponBact->slave_node));
                    WeaponBact->master = NULL;
                    }

                _get( ActualWeapon, YMA_AutoNode, &node );
                _AddTail( (struct List *) &(RifleMan->auto_list),
                          (struct Node *) node );

                /*** Schütze eintragen ***/
                _set( ActualWeapon, YMA_RifleMan, RifleMan );
                break;
            }
        }

    return( TRUE );
}


BOOL yw_DestroyPlayer( struct ypaworld_data *ywd, char *name )
{
    /* ----------------------------------------------------------------
    ** Schale für das Killen eines Spielers. Wendet nicht nur die
    ** Methode selbst an, sondern löscht den Eintrag aus dem player2-
    ** Array. Das player-Array können wir uns schenken, weil es eh erst
    ** während INITNETWORK ausgefüllt wird. Wichtig sind die Fälle,
    ** äußere Daten internen Offsets angepaßt werden müssen.
    ** --------------------------------------------------------------*/

    struct getplayerdata_msg gpd;
    struct destroyplayer_msg dp;
    int    found_number = -1;

    gpd.askmode = GPD_ASKNUMBER;
    gpd.number  = 0;

    /* ---------------------------------------------------------------
    ** Wir suchen den Spieler über GPD, weil in player2 der Name nicht
    ** steht, wir aber genau diesen Eintrag löschen müssen.
    ** -------------------------------------------------------------*/
    while( _methoda( ywd->nwo, NWM_GETPLAYERDATA, &gpd ) ) {

        if( stricmp( gpd.name, name ) == 0 ) {

            /*** Der isses. raus und löschen ***/
            found_number = gpd.number;
            break;
            }

        gpd.number++;
        }

    if( -1 != found_number ) {

        int i;

        for( i = found_number; i < (MAXNUM_PLAYERS-1); i++ )
            ywd->gsr->player2[ i ] = ywd->gsr->player2[ i + 1 ];
        }

    /*** Nun richtig Spieler löschen ***/
    dp.name = name;
    return( (BOOL) _methoda( ywd->nwo, NWM_DESTROYPLAYER, &dp ) );
}

void yw_DrawNetworkStatusInfo( struct ypaworld_data *ywd )
{
    /*** Zeichnet Informationen zum Staus der netzverbindung ***/
    char   buffer[ 2000 ];
    char  *str;
    struct VFMFont *font;
    struct rast_text rt;
    BOOL    draw_something = FALSE;
    char   t[16][300];
    int     draw_lines, i, j;

    str  = buffer;
    font = ywd->Fonts[ FONTID_TYPE_PS ];

    new_font( str, FONTID_TYPE_PS );
    pos_abs(str, (font->height -(ywd->DspXRes>>1)),
                 (font->height -(ywd->DspYRes>>1)));

    /*** verbindung unterbrochen ? ***/
    if( ywd->gsr->disconnected ) {

        }
    else {

        /*** 2. Schlechte Verbindung? ***/
        if( ywd->gsr->trouble_count > 0 ) {

            if( (ywd->TLMsg.global_time / 300) & 1 )
                put( str, 'A' );
            }
        }

    eos( str );

    _methoda(ywd->GfxObject,RASTM_Begin2D,NULL);
    rt.string = buffer;
    rt.clips  = NULL;
    _methoda(ywd->GfxObject,RASTM_Text,&rt);
    _methoda(ywd->GfxObject,RASTM_End2D,NULL);
    
    /*** neues System ***/
    if( ywd->gsr->network_trouble ) {

        draw_something = TRUE;

        switch( ywd->gsr->network_trouble ) {
                
            case NETWORKTROUBLE_LATENCY:
            
                if( ywd->gsr->is_host ) {
                    sprintf( t[0], "%s\0", ypa_GetStr( GlobalLocaleHandle, STR_YPAERROR_HOSTLATENCY1,
                             "HOST: LATENCY PROBLEMS.")); 
                    sprintf( t[1], "%s %d\0", ypa_GetStr( GlobalLocaleHandle, STR_YPAERROR_HOSTLATENCY2,
                             "PLEASE WAIT"), 
                             ywd->gsr->network_trouble_count);
                    }
                else {
                     sprintf( t[0], "%s\0", ypa_GetStr( GlobalLocaleHandle, STR_YPAERROR_CLIENTLATENCY1,
                             "CLIENT: LATENCY PROBLEMS.")); 
                     sprintf( t[1], "%s %d\0", ypa_GetStr( GlobalLocaleHandle, STR_YPAERROR_CLIENTLATENCY2,
                             "PLEASE WAIT"), 
                             ywd->gsr->network_trouble_count);
                    }
                
                draw_lines = 2;
                             
                break;
                
            case NETWORKTROUBLE_KICKOFF_YOU:
            
                sprintf( t[0], "%s",  ypa_GetStr( GlobalLocaleHandle,
                     STR_YPAERROR_KICKOFF_YOU1, "YOU ARE KICKED OFF BECAUSE NETTROUBLE") );
                sprintf( t[1], "%s %d\0", ypa_GetStr( GlobalLocaleHandle, STR_YPAERROR_KICKOFF_YOU2,
                         "LEVEL FINISHES AUTOMATICALLY"), 
                         ywd->gsr->network_trouble_count/1000);
                         
                draw_lines = 2;
                break;
                
            case NETWORKTROUBLE_KICKOFF_PLAYER:
            
                sprintf( t[0], "%s", ypa_GetStr( GlobalLocaleHandle,
                     STR_YPAERROR_KICKOFF_PLAYER1, "FOLLOWING PLAYERES WERE REMOVED") );
                sprintf( t[1], "%s\0", ypa_GetStr( GlobalLocaleHandle, STR_YPAERROR_KICKOFF_PLAYER2,
                         "BECAUSE THEY HAD NETWORK PROBLEMS"));
                j = 2;
                for( i = 0; i < MAXNUM_OWNERS; i++ ) {
                
                    if( ywd->gsr->player[ i ].was_killed & WASKILLED_SHOWIT ) {
                        strcpy( t[j++], ywd->gsr->player[ i ].name );
                        }
                    }
                    
                draw_lines = j;          
                break;
                
            case NETWORKTROUBLE_WAITINGFORPLAYER:
            
                sprintf( t[0], "%s",  ypa_GetStr( GlobalLocaleHandle,
                     STR_YPAERROR_WAITINGFORPLAYER1, "NO CONNECTION TO FOLLOWING PLAYERS") );
                sprintf( t[1], "%s\0", ypa_GetStr( GlobalLocaleHandle, STR_YPAERROR_WAITINGFORPLAYER2,
                         "FINISH IF PROBLEM CANNOT NE SOLVED"));
                
                j = 2;
                for( i = 0; i < MAXNUM_OWNERS; i++ ) {
                    if( ((ywd->TLMsg.global_time - ywd->gsr->player[i].lastmsgtime) > WAITINGFORPLAYER_TIME) &&
                        (ywd->gsr->player[i].ready_to_play) &&
                        (stricmp(ywd->gsr->player[i].name, ywd->gsr->NPlayerName) != 0))  
                        strcpy( t[j++],ywd->gsr->player[i].name );
                    }              
                draw_lines = j;
                break;
            }    
        }
    else {
    
        /*** noch Meldung, dass alles ok anzeigen ***/
        if( ywd->gsr->network_allok_count > 0 ) {
        
            draw_something = TRUE;
            draw_lines     = 1;
                
            switch( ywd->gsr->network_allok ) {
            
                case ENDTROUBLE_ALLOK:
                
                    strcpy( t[0], ypa_GetStr( GlobalLocaleHandle, STR_YPAERROR_ALLOK,
                               "NETWORK IS NOW OK" ));
                    break;
                    
                case ENDTROUBLE_TIMEDOUT:
                
                    strcpy( t[0], ypa_GetStr( GlobalLocaleHandle, STR_YPAERROR_TIMEDOUT,
                               "THERE WAS NO CHANCE TO SOLVE THIS PROBLEM" ));
                    break;
                    
                case ENDTROUBLE_UNKNOWN:
                    
                    /*** Loesung hat keinen speziellen Text ***/
                    strcpy( t[0], " ");
                    draw_something = FALSE;
                    break;
                    
                default:
                
                    strcpy( t[0],"???");
                    break;
                }
            }
        }
        
    if( draw_something ) {
    
        WORD xpos, ypos, breite;
        char *str, *h;
        struct VFMFont *fnt = ywd->Fonts[ FONTID_DEFAULT ];

        str = buffer;
    
        new_font( str, FONTID_DEFAULT );
        breite = ywd->DspXRes/2;
        xpos   = (ywd->DspXRes - breite)/2 - (ywd->DspXRes>>1);
        ypos   = 12 - (ywd->DspYRes>>1);
        pos_abs(str, xpos, ypos);                
        
        h = ypa_GetStr( GlobalLocaleHandle, STR_YPAERROR_NETSTATUS, "NETZWERKSTATUS");

        /*** Erste zeile mit Überschrift ***/
        str = yw_BuildReqTitle( ywd, xpos, ypos, breite,  h, str, 0, 0 );
        new_line( str );
        new_font( str, FONTID_DEFAULT );

        /*** Zeilen mit Messages ***/
        for( i = 0; i < draw_lines; i++ ) {
        
            put( str, '{' );
            
            #ifdef __DBCS__
            freeze_dbcs_pos( str );
            #else
            str = yw_StpCpy( t[i], str );
            #endif
            
            lstretch_to( str, breite - fnt->fchars[ '}' ].width);
            put( str, ' ');
            put( str, '}'); 
            
            /*** jetzt erst den DBCS-Text ***/
            #ifdef __DBCS__
            put_dbcs( str, breite - 2 * fnt->fchars[ 'W' ].width, DBCSF_CENTER, t[i] );
            #endif
            new_line( str );
            }
                        
        /*** Zeile mit abschluß ***/
        off_vert( str, fnt->height - 1 );
        put( str, 'x');
        lstretch_to( str, breite - fnt->fchars[ 'z' ].width);
        put( str, 'y');
        put( str, 'z');

        /*** Abschluß ***/
        eos( str );

        _methoda(ywd->GfxObject,RASTM_Begin2D,NULL);
        rt.string = buffer;
        rt.clips  = NULL;
        _methoda(ywd->GfxObject,RASTM_Text,&rt);
        _methoda(ywd->GfxObject,RASTM_End2D,NULL);
        }
}


void yw_SendAnnounceQuit( struct ypaworld_data *ywd )
{
    /* --------------------------------------------------------------
    ** AnnounceQuit ist eine Meldung, die Anzeigt, dass ein folgendes
    ** DESTROYPLAYER eine normale Useraktion und kein Rausschmiss ist
    ** ------------------------------------------------------------*/

    struct sendmessage_msg sm;
    struct ypamessage_announcequit aq;

    aq.generic.message_id = YPAM_ANNOUNCEQUIT;
    aq.generic.owner      = ywd->gsr->NPlayerOwner;

    sm.receiver_id        = NULL;
    sm.receiver_kind      = MSG_ALL;
    sm.data               = &aq;
    sm.data_size          = sizeof( aq );
    sm.guaranteed         = TRUE;
    _methoda( ywd->world, YWM_SENDMESSAGE, &sm );
}


BOOL yw_OpenNetScript(void)
{
    /*** ermittelt Name zum Script fuer die folgende Session ***/
    FILE *f;
    char wd[ 300 ], number[ 100 ];
    struct dirent *dir, *entry;

    /* -----------------------------------------------------------
    ** Wenn nicht gewuenscht, dann auch nicht initialisieren, dann
    ** ist der NETLOGCOUNT nicht initialisiert und somit kommen
    ** auch keine Messages durch. GetConfigItem wurde schon in
    ** gsinit.c gemacht.
    ** ---------------------------------------------------------*/
    if( !yw_ConfigItems[3].data ) return( TRUE );

    /*** Wenn Verzeichnis noch nicht existiert, dann erzeugen ***/
    memset( wd, 0, 300 );
    getcwd( wd, 300 );
    strcat( wd, "\\env\\debug" );
    if( dir = opendir( wd ) ) {
        closedir( dir );
        }
    else {
        if( mkdir( wd ))
            return( FALSE );
        }

    /* ---------------------------------------------------------
    ** hoechste derzeitige Nummer fuer net_log.txt suchen. Keine
    ** Nucleus-Routinen, die machen die Filenamen kaputt
    ** -------------------------------------------------------*/
    if( dir = opendir( wd ) ) {
        while( entry = readdir( dir ) ) {

            if( strnicmp( entry->d_name, "net_log",7 ) == 0 ) {

                /*** ein Netz-Logfile ***/
                strcpy( number, &(entry->d_name[7]) );
                strtok( number, "." );
                if( NETLOGCOUNT < atol( number ) )
                    NETLOGCOUNT = atol( number );
                }
            }
        closedir( dir );
        }
    NETLOGCOUNT++;

    sprintf( NETLOGNAME, "env:debug\\net_log%d.txt", NETLOGCOUNT );
    if( f = _FOpen( NETLOGNAME, "w" )) {
        _FClose( f );
        return( TRUE );
        }
    else
        return( FALSE );
}


BOOL yw_NetLog( char* string, ... )
{
    FILE *f;

    /*** Schon initialisiert? ***/
    if( 0 == NETLOGCOUNT ) return( FALSE );

    if( f = _FOpen( NETLOGNAME, "a" )) {

        va_list arglist;

        va_start(arglist, string);
        vfprintf( f, string, arglist );

        _FClose( f );

        va_end( arglist);
        return( TRUE );
        }
    else
        return( FALSE );
}


ULONG yw_CheckSumOfThisFile( char *filename )
{
    ULONG checksum = 0, i;
    APTR  *f;
    char buffer[ 300 ]; 
    
    if( f = _FOpen( filename, "r" )) {
        
        memset( buffer, 0, 300 );
        while( _FGetS( buffer, 300, f ) ) {
        
            for( i = 0; i < strlen(buffer); i++ )
                if( (buffer[i]>32) && (buffer[i]<128) )
                    checksum += buffer[ i ];
            } 
        
        _FClose( f );
        }
        
    return( checksum );   
}

void yw_SendCheckSum( struct ypaworld_data *ywd, ULONG levelnum )
{
    /* -------------------------------------------------------------
    ** testet alle wichtigen scripts incl. das des angegebenen Levels
    ** und sendet die daten an alle anderen. Dies passiert immer dann,
    ** wenn der level klar ist.
    ** alle Zeichen kleiner 33 werden ignoriert.
    ** -----------------------------------------------------------*/
    
    ULONG checksum = 0;
    char  lt[ 300 ];
    struct sendmessage_msg sm;
    struct ypamessage_checksum cs;

    checksum += yw_CheckSumOfThisFile( "data:scripts/feinde.scr");
    checksum += yw_CheckSumOfThisFile( "data:scripts/user.scr");
    checksum += yw_CheckSumOfThisFile( "data:scripts/weap_f.scr");
    checksum += yw_CheckSumOfThisFile( "data:scripts/weap_u.scr");
    checksum += yw_CheckSumOfThisFile( "data:scripts/net_robo.scr");
    checksum += yw_CheckSumOfThisFile( "data:scripts/net_bldg.scr");
    checksum += yw_CheckSumOfThisFile( "data:scripts/flaks.scr");
    checksum += yw_CheckSumOfThisFile( "data:scripts/net_ypa.scr");
    checksum += yw_CheckSumOfThisFile( "data:scripts/inetrobo.scr");
    
    sprintf( lt, "levels:multi/L%02d%02d.ldf", levelnum, levelnum );
    checksum += yw_CheckSumOfThisFile( lt );
    
    ywd->gsr->NCheckSum   = checksum;

    cs.generic.message_id = YPAM_CHECKSUM;
    cs.generic.owner      = ywd->gsr->NPlayerOwner;
    cs.checksum           = checksum;

    sm.receiver_id        = NULL;
    sm.receiver_kind      = MSG_ALL;
    sm.data               = &cs;
    sm.data_size          = sizeof( cs );
    sm.guaranteed         = TRUE;
    _methoda( ywd->world, YWM_SENDMESSAGE, &sm );
}


void yw_TellAboutCheckSum( struct ypaworld_data *ywd )
{
    BOOL  something_wrong;
    ULONG n, i, num_players;
    char  text[ MAXNUM_PLAYERS ][ 300 ];
    char  name[ 300 ];

    if( ywd->gsr->tacs_time > 0 ) {
    
        ywd->gsr->tacs_time -= ywd->gsr->frame_time;
        return;
        }
        
    ywd->gsr->tacs_time = 15000;
    num_players         = _methoda( ywd->nwo, NWM_GETNUMPLAYERS, NULL );
    n                   = 0;
    something_wrong     = FALSE;
    
    for( i = 0; i < num_players; i++ ) {
    
        if( (stricmp( ywd->gsr->NPlayerName, ywd->gsr->player2[i].name) != 0) &&
            (ywd->gsr->player2[i].checksum != ywd->gsr->NCheckSum) &&
            (ywd->gsr->player2[i].checksum != 0L) &&
            (ywd->gsr->NCheckSum           != 0L)) {
            
            something_wrong = TRUE;
            sprintf( text[ n ], "%s %s\0", ywd->gsr->player2[i].name, 
                     ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_HASOTHERFILES,
                                 "HAS OTHER FILES THAN YOU"));
            n++;
            }
        }
        
    if( something_wrong ) {  
    
        sprintf( name, "%s\0", ypa_GetStr( GlobalLocaleHandle,
                 STR_NGADGET_COMPUTER, "COMPUTER"));
        
        for( i = 0; i < n; i++ ) 
            yw_AddMessageToBuffer( ywd, name, text[i]);
        }
}

char *yw_CorpsesInCellar( struct GameShellReq *GSR )
{
/* -------------------------------------------------------
** checkt aus, ob da Vehicle sind, die schon seit Urzeiten
** nicht mehr upgedated wurden. Wenn dem so ist, dann
** mal was zurueckgeben, damit ein Update angefordert
** werden kann.
** zurueckgegeben wird der Name desjenigen, dessen Vehicle
** zu alt sind.
** -----------------------------------------------------*/

    struct OBNode *robo;
    struct Bacterium *found = NULL;
    LONG    time = 180000;
    
    /*** Ist es wiedermal an der Zeit, einen test zu machen? ***/
    if( (GSR->ywd->TimeStamp - GSR->corpse_check) < 100000 )
        return( NULL );
        
    GSR->corpse_check = GSR->ywd->TimeStamp; 
    
    robo = (struct OBNode *) GSR->ywd->CmdList.mlh_Head;
    
    while( robo->nd.mln_Succ ) {
    
        /*** Ein Schattenvehicle? ***/
        if( (robo->bact->Owner != 0) &&
            (robo->bact->Owner != GSR->NPlayerOwner) ) {
            
            struct OBNode *commander = (struct OBNode *) robo->bact->slave_list.mlh_Head;
            while( commander->nd.mln_Succ ) {
            
                struct OBNode *slave = (struct OBNode *) commander->bact->slave_list.mlh_Head;
                while( slave->nd.mln_Succ ) {
                
                    if( (GSR->ywd->TimeStamp - slave->bact->last_frame) > time ) {
                    
                        found = slave->bact;
                        break;
                        }
                    slave = (struct OBNode *) slave->nd.mln_Succ;
                    }
                    
                if( found ) break;
                
                if( (GSR->ywd->TimeStamp - commander->bact->last_frame) > time ) {
                
                    found = commander->bact;
                    break;
                    }
                commander = (struct OBNode *) commander->nd.mln_Succ;
                }
                
            if( found ) break;
            
            if( (GSR->ywd->TimeStamp - robo->bact->last_frame) > time ) {
            
                found = robo->bact;
                break;
                }
            }
        robo = (struct OBNode *) robo->nd.mln_Succ;
        }
             
    /*** ein Problem gefunden? ***/            
    if( found ) {
    
        /*** Welcher Name gehoert zu diesem Vehicle? ***/
        char *name = GSR->player[ found->Owner ].name;
        yw_NetLog("\n+++ CC: found old vehicle id %d, class %d, owner %d at time %d. Request update\n",
                   found->ident, found->BactClassID, found->Owner, GSR->ywd->TimeStamp/1000 ); 
        return( name );
        }
    else 
        return( NULL );
}         

