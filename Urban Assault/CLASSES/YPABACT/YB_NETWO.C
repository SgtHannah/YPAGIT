/*
**  $Source: $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:24:47 $
**  $Locker: $
**  $Author: $
**
**  Die netzwerkroutinen
**
**  (C) Copyright 1997 by Andreas Flemming
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
#include "ypa/ypagunclass.h"
#include "ypa/ypamissileclass.h"
#include "input/input.h"
#include "audio/audioengine.h"

#ifdef __NETWORK__

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine
_extern_use_audio_engine



/*-----------------------------------------------------------------*/
void yb_m_mul_m(struct flt_m3x3 *m1, struct flt_m3x3 *m2, struct flt_m3x3 *m);
void yb_rot_round_lokal_z( struct Bacterium *bact, FLOAT angle);
void yb_rot_round_lokal_y( struct Bacterium *bact, FLOAT angle);
void yb_OutOfWorld_Exception( struct ypabact_data *ybd );
void yb_rot_vec_round_vec( struct flt_triple *rot, struct flt_triple *vec, FLOAT angle );
void yb_m_mul_v(struct flt_m3x3 *m, struct flt_triple *v1, struct flt_triple *v);
void yb_rot_round_lokal_y2( struct flt_m3x3 *dir,  FLOAT angle);
void yb_DoWhilePlasma( struct ypabact_data *ybd, LONG sc_time, 
                       LONG frame_time, FLOAT sc_fct );
void yb_DoExtraVPHacks( struct ypabact_data *ybd, struct trigger_logic_msg *msg );
/*-----------------------------------------------------------------*/

_dispatcher(void, yb_YBM_TR_NETLOGIC, struct trigger_logic_msg *msg)
/*
**  FUNCTION
**      Wird einmal pro Frame auf jedes Bakterien-Object
**      angewendet.
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      09-Jun-95   floh    created
**      11-Jun-95   floh    (1) erweitert um GetSectorInfo-Stuff
**                          (2) wird jetzt an alle Untergebenen weitergereicht
**      27-Apr-96   floh    + Audio-Support
*/
{
    ULONG  slave_count_store;
    struct getsectorinfo_msg gsi_msg;
    struct ypabact_data *ybd = INST_DATA(cl,o);
    struct MinList *ls;
    struct MinNode *nd, *next;
    struct Cell *old_sector;
    struct flt_m3x3 *tfmx;
    struct OBNode *waffe;
    Object *slave;
    
      
    yb_DoExtraVPHacks( ybd, msg );

    /* -----------------------------------------------------------------
    ** GetSectorInfo-Stuff auf neue Position. Das Sektor-Handling müssen
    ** wir machen, weil wir sonst nicht dargestellt werden!
    ** ---------------------------------------------------------------*/
    gsi_msg.abspos_x = ybd->bact.pos.x;
    gsi_msg.abspos_z = ybd->bact.pos.z;
    if ( !_methoda(ybd->world, YWM_GETSECTORINFO, &gsi_msg)) {

        /*
        ** Wir waren außerhalb der Welt. Ich rufe eine Ausnamebehandlung
        ** auf, die das Objekt wieder korrekt in die Welt setzt, so daß
        ** ein erneuter GETSECTORINFO-Aufruf funktionieren muß!
        */

        yb_OutOfWorld_Exception( ybd );
        gsi_msg.abspos_x = ybd->bact.pos.x;
        gsi_msg.abspos_z = ybd->bact.pos.z;
        _methoda( ybd->world, YWM_GETSECTORINFO, &gsi_msg );
        }

    old_sector = ybd->bact.Sector;

    ybd->bact.SectX    = gsi_msg.sec_x;
    ybd->bact.SectY    = gsi_msg.sec_y;
    ybd->bact.relpos.x = gsi_msg.relpos_x;
    ybd->bact.relpos.z = gsi_msg.relpos_z;
    ybd->bact.Sector   = gsi_msg.sector;

    /*** Sektor-Sprung? ***/
    if (ybd->bact.Sector != old_sector) {
        _Remove((struct Node *) &(ybd->bact.SectorNode));
        _AddTail((struct List *) &(ybd->bact.Sector->BactList),
                 (struct Node *) &(ybd->bact.SectorNode));
        }

    /*** Kontrolle an User bzw. AI-Engine ***/
    ybd->bact.internal_time  += msg->frame_time;
    _methoda(o, YBM_SHADOWTRIGGER, msg);

    /* -------------------------------------------------------
    ** Alle Waffen nun richtig triggern. Achtung, aus
    ** Sicherheitsgruenden setze ich den Rifleman jedesmal neu
    ** -----------------------------------------------------*/
    waffe = (struct OBNode *) ybd->bact.auto_list.mlh_Head;
    while( waffe->nd.mln_Succ ) {

        _set( waffe->o, YMA_RifleMan, &(ybd->bact) );
        //_methoda( waffe->o, YBM_TR_NETLOGIC, msg );
        _methoda( waffe->o, YBM_TR_LOGIC, msg );
        waffe = (struct OBNode *) waffe->nd.mln_Succ;
        }

    /*** tote Waffen raus ***/
    yb_RemoveDeadWeapon( ybd );
    
    /*** Position ***/
    ybd->bact.tf.loc  = ybd->bact.pos;

    /*** transponierte Matrix schreiben ***/

    /*** Mit Skalierung ***/
    if( ybd->bact.ExtraState & EXTRA_SCALE ) {

        tfmx = &(ybd->bact.tf.loc_m);
        tfmx->m11 = ybd->bact.dir.m11 * ybd->bact.scale_x;
        tfmx->m12 = ybd->bact.dir.m21 * ybd->bact.scale_y;
        tfmx->m13 = ybd->bact.dir.m31 * ybd->bact.scale_z;

        tfmx->m21 = ybd->bact.dir.m12 * ybd->bact.scale_x;
        tfmx->m22 = ybd->bact.dir.m22 * ybd->bact.scale_y;
        tfmx->m23 = ybd->bact.dir.m32 * ybd->bact.scale_z;

        tfmx->m31 = ybd->bact.dir.m13 * ybd->bact.scale_x;
        tfmx->m32 = ybd->bact.dir.m23 * ybd->bact.scale_y;
        tfmx->m33 = ybd->bact.dir.m33 * ybd->bact.scale_z;
        }
    else {

        tfmx = &(ybd->bact.tf.loc_m);
        tfmx->m11 = ybd->bact.dir.m11;
        tfmx->m12 = ybd->bact.dir.m21;
        tfmx->m13 = ybd->bact.dir.m31;

        tfmx->m21 = ybd->bact.dir.m12;
        tfmx->m22 = ybd->bact.dir.m22;
        tfmx->m23 = ybd->bact.dir.m32;

        tfmx->m31 = ybd->bact.dir.m13;
        tfmx->m32 = ybd->bact.dir.m23;
        tfmx->m33 = ybd->bact.dir.m33;
        }


    /*** Methode an Untergebene weitergeben ***/
    slave_count_store = msg->slave_count;
    msg->slave_count  = 0;
    ls = &(ybd->bact.slave_list);
    nd = ls->mlh_Head;
    while( nd->mln_Succ ) {

        /*** Nachfolger vorher merken, falls wir uns ausklinken ***/
        next = nd->mln_Succ;

        /*** Triggern ***/
        slave = ((struct OBNode *)nd)->o;
        _methoda(slave, YBM_TR_NETLOGIC, msg);
        msg->slave_count++;

        /*** Nachfolger ***/
        nd = next;
        }
    msg->slave_count = slave_count_store;

    /*** AUDIO: SoundCarrier updaten und refreshen ***/
    ybd->bact.sc.pos   = ybd->bact.pos;

    ybd->bact.sc.vec.x = ybd->bact.dof.x * ybd->bact.dof.v;
    ybd->bact.sc.vec.y = ybd->bact.dof.y * ybd->bact.dof.v;
    ybd->bact.sc.vec.z = ybd->bact.dof.z * ybd->bact.dof.v;
    _RefreshSoundCarrier(&(ybd->bact.sc));

    /*** Ende ***/
}


_dispatcher(void, yb_YBM_SHADOWTRIGGER, struct trigger_logic_msg *msg)
{
    /*** Verteiler je nach Näherungsverfahren ***/
    struct ypaworld_data *ywd;
    struct ypabact_data  *ybd = INST_DATA( cl, o );

    ywd = INST_DATA( ((struct nucleusdata *)ybd->world)->o_Class, ybd->world);

    if( ywd->interpolate )
        _methoda( o, YBM_SHADOWTRIGGER_I, msg );
    else
        _methoda( o, YBM_SHADOWTRIGGER_E, msg );
}


/*-----------------------------------------------------------------*/
_dispatcher(void, yb_YBM_SHADOWTRIGGER_E, struct trigger_logic_msg *msg)
{
/*
**  FUNCTION
**
**      Dies ist die Triggermethode für ein AI-Vehicle, welches auf
**      einem anderen Rechner berechnet wurde. Es kommen in mehr oder
**      weniger regelmäßigen Abständen Positionen und Ausrichtungen rein.
**
**      Irgendwoher (hier mit der message oder bereits aus meiner LID,
**      wenn jemand anderes dort schon reingeschrieben hat), bekomme
**      ich Daten, aus denen ich position und Ausrichtung extrapoliere.
**
**      Es gibt 2 Extrapolationsmodi, einmal pos annähern, andermal speed.
**      ich vermute, daß bei kleinen zeiten erstere, sonst letzere exakter
**      ist. Rechnezeitvorteile dürften keine Rolle spielen.
**
**      Zum merken gibt es in der LID:
**      d_pos       wegänderung pro zeit
**      d_speed     Geschwindigkeitsänderung pro Zeit
**      d_matrix    Matrizenänderung/-drehung pro Zeit
**
**      Weiterhin nutze ich pos, old_pos und dof
**
**      Zur berechnung muß ich beachten, daß alles mehr oder weniger
**      "physikalisch", also SI-konform ist, lediglich die Wege müssen
**      durch METER_SIZE geteilt werden, um auf m zu kommen.
**
**  INPUTS
**
**
**  RESULTS
**
**  CHANGED
**
**       7-Feb-97   8100000C    created
*/

    struct ypabact_data *ybd;
    FLOAT  frame_time, time_since_upd, len;
    struct flt_triple new_speed;

    ybd = INST_DATA( cl, o );

    frame_time = ((FLOAT)msg->frame_time) / 1000.0;

    /*** Wieviel zeit ist seit Aktualisierung der Werte vergangen? ***/
    time_since_upd = ((FLOAT)(msg->global_time - ybd->bact.last_frame)) / 1000.0;

    if( time_since_upd > 0.0 ) {

        ULONG rgun;

        /* --------------------------------------------------------------
        ** Denn nur dann macht es Sinn!
        ** Zuerst kommt die matrix. dmatrix * dt wird einfach aufaddiert.
        ** d_matrix berechne ich jetzt aus einer Änderung von d_matrix,
        ** also einer Winkel- oder matrixbeschleunigung.
        ** ------------------------------------------------------------*/
        ybd->bact.dir.m11 += ybd->bact.d_matrix.m11 * frame_time;
        ybd->bact.dir.m12 += ybd->bact.d_matrix.m12 * frame_time;
        ybd->bact.dir.m13 += ybd->bact.d_matrix.m13 * frame_time;
        ybd->bact.dir.m21 += ybd->bact.d_matrix.m21 * frame_time;
        ybd->bact.dir.m22 += ybd->bact.d_matrix.m22 * frame_time;
        ybd->bact.dir.m23 += ybd->bact.d_matrix.m23 * frame_time;
        ybd->bact.dir.m31 += ybd->bact.d_matrix.m31 * frame_time;
        ybd->bact.dir.m32 += ybd->bact.d_matrix.m32 * frame_time;
        ybd->bact.dir.m33 += ybd->bact.d_matrix.m33 * frame_time;

        /* ------------------------------------------------------------------
        ** Rechtwinklig machen. Zuerst normieren wir x. Dann wird in der
        ** x-y-Ebene y auf x bis zur Rechtwinkligkeit zugedreht und ebenfalls
        ** normiert. Anschließend ergibt sich z noch aus x X y
        ** ----------------------------------------------------------------*/

        /*** Nachnormieren ***/
        len = nc_sqrt( ybd->bact.dir.m11 * ybd->bact.dir.m11 + ybd->bact.dir.m12 *
                       ybd->bact.dir.m12 + ybd->bact.dir.m13 * ybd->bact.dir.m13);
        if( len > 0.001 ) {
            ybd->bact.dir.m11 /= len; ybd->bact.dir.m12 /= len; ybd->bact.dir.m13 /= len;
            }
        else {
            ybd->bact.dir.m11 = 1.0; ybd->bact.dir.m12 = 0.0; ybd->bact.dir.m13 = 0.0;
            }

        len = nc_sqrt( ybd->bact.dir.m21 * ybd->bact.dir.m21 + ybd->bact.dir.m22 *
                       ybd->bact.dir.m22 + ybd->bact.dir.m23 * ybd->bact.dir.m23);
        if( len > 0.001 ) {
            ybd->bact.dir.m21 /= len; ybd->bact.dir.m22 /= len; ybd->bact.dir.m23 /= len;
            }
        else {
            ybd->bact.dir.m21 = 0.0; ybd->bact.dir.m22 = 1.0; ybd->bact.dir.m23 = 0.0;
            }

        len = nc_sqrt( ybd->bact.dir.m31 * ybd->bact.dir.m31 + ybd->bact.dir.m32 *
                       ybd->bact.dir.m32 + ybd->bact.dir.m33 * ybd->bact.dir.m33);
        if( len > 0.001 ) {
            ybd->bact.dir.m31 /= len; ybd->bact.dir.m32 /= len; ybd->bact.dir.m33 /= len;
            }
        else {
            ybd->bact.dir.m31 = 0.0; ybd->bact.dir.m32 = 0.0; ybd->bact.dir.m33 = 1.0;
            }

        /* ----------------------------------------------------
        ** geschwindigkeit extrapolieren, anschließend Position
        ** neu ermitteln
        ** --------------------------------------------------*/
        new_speed.x = ybd->bact.dof.x * ybd->bact.dof.v +
                      time_since_upd * ybd->bact.d_speed.x;
        new_speed.y = ybd->bact.dof.y * ybd->bact.dof.v +
                      time_since_upd * ybd->bact.d_speed.y;
        new_speed.z = ybd->bact.dof.z * ybd->bact.dof.v +
                      time_since_upd * ybd->bact.d_speed.z;

        rgun = FALSE;
        if( ybd->bact.BactClassID == BCLID_YPAGUN )
            _get( o, YGA_RoboGun, &rgun );

        /*** Denn bei Guns gibt es keine Positionsänderung ***/
        if( (BCLID_YPAGUN  == ybd->bact.BactClassID) && (!rgun) ) {

            /*** Flak-Gun: Nix machen ***/
            }
        else {

            ybd->bact.pos.x += new_speed.x * frame_time * METER_SIZE;
            ybd->bact.pos.y += new_speed.y * frame_time * METER_SIZE;
            ybd->bact.pos.z += new_speed.z * frame_time * METER_SIZE;
            }

        /*** Wegen Weltrand ***/
        _methoda( o, YBM_CORRECTPOSITION, NULL );

        /*** Gelandete und Panzer nachkorrigieren. Das machen wir immer ***/
        if( ybd->bact.ExtraState & EXTRA_LANDED ) {

            struct intersect_msg inter;
            inter.pnt.x = ybd->bact.pos.x;         inter.pnt.y = ybd->bact.pos.y;
            inter.pnt.z = ybd->bact.pos.z;         inter.vec.x = ybd->bact.dir.m21 * 200;
            inter.vec.y = ybd->bact.dir.m22 * 200; inter.vec.z = ybd->bact.dir.m23 * 200;
            inter.flags = 0;
            _methoda( ybd->world, YWM_INTERSECT, &inter );
            if( inter.insect ) {

                ybd->bact.pos.x = inter.ipnt.x - ybd->bact.dir.m21 * ybd->bact.over_eof;
                ybd->bact.pos.y = inter.ipnt.y - ybd->bact.dir.m22 * ybd->bact.over_eof;
                ybd->bact.pos.z = inter.ipnt.z - ybd->bact.dir.m23 * ybd->bact.over_eof;
                }
            }
        }
}


_dispatcher(void, yb_YBM_SHADOWTRIGGER_I, struct trigger_logic_msg *msg)
{
/*
**  FUNCTION
**
**      
**
**  INPUTS
**
**
**  RESULTS
**
**  CHANGED
**
**       7-Feb-97   8100000C    created
*/

    struct ypabact_data *ybd;
    FLOAT  frame_time, time_since_upd, len;

    ybd = INST_DATA( cl, o );

    frame_time = ((FLOAT)msg->frame_time) / 1000.0;

    /*** Wieviel zeit ist seit Aktualisierung der Werte vergangen? ***/
    time_since_upd = ((FLOAT)(msg->global_time - ybd->bact.last_frame)) / 1000.0;

    if( time_since_upd > 0.0 ) {

        FLOAT a;
        struct flt_triple d, y;

        /* --------------------------------------------------------------
        ** Denn nur dann macht es Sinn!
        ** Zuerst kommt die matrix. dmatrix * dt wird einfach aufaddiert.
        ** d_matrix berechne ich jetzt aus einer Änderung von d_matrix,
        ** also einer Winkel- oder matrixbeschleunigung.
        ** ------------------------------------------------------------*/
        ybd->bact.dir.m11 += ybd->bact.d_matrix.m11 * frame_time;
        ybd->bact.dir.m12 += ybd->bact.d_matrix.m12 * frame_time;
        ybd->bact.dir.m13 += ybd->bact.d_matrix.m13 * frame_time;
        ybd->bact.dir.m21 += ybd->bact.d_matrix.m21 * frame_time;
        ybd->bact.dir.m22 += ybd->bact.d_matrix.m22 * frame_time;
        ybd->bact.dir.m23 += ybd->bact.d_matrix.m23 * frame_time;
        ybd->bact.dir.m31 += ybd->bact.d_matrix.m31 * frame_time;
        ybd->bact.dir.m32 += ybd->bact.d_matrix.m32 * frame_time;
        ybd->bact.dir.m33 += ybd->bact.d_matrix.m33 * frame_time;

        /*** Nachnormieren ***/
        len = nc_sqrt( ybd->bact.dir.m11 * ybd->bact.dir.m11 + ybd->bact.dir.m12 *
                       ybd->bact.dir.m12 + ybd->bact.dir.m13 * ybd->bact.dir.m13);
        if( len > 0.0001 ) {
            ybd->bact.dir.m11 /= len; ybd->bact.dir.m12 /= len; ybd->bact.dir.m13 /= len;
            }
        else {
            ybd->bact.dir.m11 = 1.0; ybd->bact.dir.m12 = 0.0; ybd->bact.dir.m13 = 0.0;
            }

        len = nc_sqrt( ybd->bact.dir.m21 * ybd->bact.dir.m21 + ybd->bact.dir.m22 *
                       ybd->bact.dir.m22 + ybd->bact.dir.m23 * ybd->bact.dir.m23);
        if( len > 0.0001 ) {
            ybd->bact.dir.m21 /= len; ybd->bact.dir.m22 /= len; ybd->bact.dir.m23 /= len;
            }
        else {
            ybd->bact.dir.m21 = 0.0; ybd->bact.dir.m22 = 1.0; ybd->bact.dir.m23 = 0.0;
            }

        /*** Ab hier neuer Weg ***/

        /*** Winkel ermitteln ***/
        a = nc_acos( ybd->bact.dir.m11 * ybd->bact.dir.m21 +
                     ybd->bact.dir.m12 * ybd->bact.dir.m22 +
                     ybd->bact.dir.m13 * ybd->bact.dir.m23 );
        a = PI/2 - a;

        /*** Drehachse ermitteln (x X y) ***/
        d.x = ybd->bact.dir.m12 * ybd->bact.dir.m23 -
              ybd->bact.dir.m13 * ybd->bact.dir.m22;
        d.y = ybd->bact.dir.m13 * ybd->bact.dir.m21 -
              ybd->bact.dir.m11 * ybd->bact.dir.m23;
        d.z = ybd->bact.dir.m11 * ybd->bact.dir.m22 -
              ybd->bact.dir.m12 * ybd->bact.dir.m21;

        /*** Rotieren ***/
        y.x = ybd->bact.dir.m21;
        y.y = ybd->bact.dir.m22;
        y.z = ybd->bact.dir.m23;
        yb_rot_vec_round_vec( &d, &y, a );
        ybd->bact.dir.m21 = y.x;
        ybd->bact.dir.m22 = y.y;
        ybd->bact.dir.m23 = y.z;

        /*** z = x X y ***/
        ybd->bact.dir.m31 = ybd->bact.dir.m12 * ybd->bact.dir.m23 -
                            ybd->bact.dir.m13 * ybd->bact.dir.m22;
        ybd->bact.dir.m32 = ybd->bact.dir.m13 * ybd->bact.dir.m21 -
                            ybd->bact.dir.m11 * ybd->bact.dir.m23;
        ybd->bact.dir.m33 = ybd->bact.dir.m11 * ybd->bact.dir.m22 -
                            ybd->bact.dir.m12 * ybd->bact.dir.m21;

        //Rest von alter Sache. Gegenstück zu "Ab hier..."
        //len = nc_sqrt( ybd->bact.dir.m31 * ybd->bact.dir.m31 + ybd->bact.dir.m32 *
        //               ybd->bact.dir.m32 + ybd->bact.dir.m33 * ybd->bact.dir.m33);
        //if( len > 0.0001 ) {
        //    ybd->bact.dir.m31 /= len; ybd->bact.dir.m32 /= len; ybd->bact.dir.m33 /= len;
        //    }
        //else {
        //    ybd->bact.dir.m31 = 0.0; ybd->bact.dir.m32 = 0.0; ybd->bact.dir.m33 = 1.0;
        //    }

        /*** Geschwindigkeit neu berechnen ***/
        //ybd->bact.dof.x *= ybd->bact.dof.v;
        //ybd->bact.dof.y *= ybd->bact.dof.v;
        //ybd->bact.dof.z *= ybd->bact.dof.v;
        //ybd->bact.dof.x += (ybd->bact.accel.x * frame_time);
        //ybd->bact.dof.y += (ybd->bact.accel.y * frame_time);
        //ybd->bact.dof.z += (ybd->bact.accel.z * frame_time);
        //ybd->bact.dof.v = nc_sqrt( ybd->bact.dof.x * ybd->bact.dof.x +
        //                           ybd->bact.dof.y * ybd->bact.dof.y +
        //                           ybd->bact.dof.z * ybd->bact.dof.z);
        //if( ybd->bact.dof.v > 0.01 ) {
        //    ybd->bact.dof.x /= ybd->bact.dof.v;
        //    ybd->bact.dof.y /= ybd->bact.dof.v;
        //    ybd->bact.dof.z /= ybd->bact.dof.v;
        //    }

        /*** Positionsänderung ***/
        ybd->bact.pos.x += ybd->bact.dof.v*ybd->bact.dof.x*frame_time*METER_SIZE;
        ybd->bact.pos.y += ybd->bact.dof.v*ybd->bact.dof.y*frame_time*METER_SIZE;
        ybd->bact.pos.z += ybd->bact.dof.v*ybd->bact.dof.z*frame_time*METER_SIZE;

        /*** Wegen Weltrand ***/
        _methoda( o, YBM_CORRECTPOSITION, NULL );
        }
}


void yb_SendSquadronStructure( struct ypaworld_data *ywd, UBYTE owner )
{
    /* ------------------------------------------------------------
    ** Sucht zuerst den Robo zum Eigentümer und füllt dann data mit
    ** den Infos zur Sache aus. Nur bis MAXNUM_DEBUGMSGITEMS!
    ** Verschickt auch gleich die Message.
    ** ----------------------------------------------------------*/
    BOOL   found = FALSE;
    struct ypamessage_debug db;
    struct sendmessage_msg sm;
    struct OBNode *robo, *commander, *slave;
    ULONG  count;

    if( owner == 0 ) return;

    robo = (struct OBNode *) ywd->CmdList.mlh_Head;
    while( robo->nd.mln_Succ ) {

        if( robo->bact->Owner == owner ) {

            found = TRUE;
            break;
            }

        robo = (struct OBNode *) robo->nd.mln_Succ;
        }

    if( !found ) return;

    count = 0;

    /*** Robo eintragen ***/
    db.data[ count++ ] = robo->bact->ident;

    commander = (struct OBNode *) robo->bact->slave_list.mlh_Head;
    while( commander->nd.mln_Succ ) {

        db.data[ count++ ] = commander->bact->ident;
        if( count >= MAXNUM_DEBUGMSGITEMS ) break;

        slave = (struct OBNode *) commander->bact->slave_list.mlh_Head;
        while( slave->nd.mln_Succ ) {

            db.data[ count++ ] = slave->bact->ident;
            if( count >= MAXNUM_DEBUGMSGITEMS ) break;

            slave = (struct OBNode *) slave->nd.mln_Succ;
            }

        db.data[ count++ ] = DBGMSG_NEWCOMMANDER;
        if( count >= MAXNUM_DEBUGMSGITEMS ) break;

        commander = (struct OBNode *) commander->nd.mln_Succ;
        }

    db.data[ count++ ] = DBGMSG_END;

    /*** Verschicken ***/
    db.generic.owner      = owner;
    db.generic.message_id = YPAM_DEBUG;

    sm.receiver_id        = NULL;
    sm.receiver_kind      = MSG_ALL;
    sm.data               = &db;
    sm.data_size          = sizeof( db );
    sm.guaranteed         = TRUE;
    _methoda( ywd->world, YWM_SENDMESSAGE, &sm );
}


void yb_m_mul_v(struct flt_m3x3 *m, struct flt_triple *v1, struct flt_triple *v)
{
/*
**  FUNCTION
**      Multipliziert Vector mit matrix.
**
**      v = m1*v1
*/

    v->x = m->m11*v1->x + m->m12*v1->y + m->m13*v1->z;
    v->y = m->m21*v1->x + m->m22*v1->y + m->m23*v1->z;
    v->z = m->m31*v1->x + m->m32*v1->y + m->m33*v1->z;
}


void yb_rot_vec_round_vec( struct flt_triple *rot, struct flt_triple *vec, FLOAT angle )
{
    FLOAT sa, ca, rot_x, rot_y, rot_z;
    struct flt_m3x3 rm;
    struct flt_triple vc;

    rot_x = rot->x;
    rot_y = rot->y;
    rot_z = rot->z;

    /* -----------------------------------------------------------------
    ** Weil die reihenfolge matrix-Vec/Matr zu anderen hier vertauscht
    ** ist, und es nach außen gleich aussehen soll, negieren wir einfach
    ** den Winkel. Das ist weder schön noch intelligent, aber selten
    ** ---------------------------------------------------------------*/
    angle = -angle;

    ca = cos( angle );
    sa = sin( -angle );

    rm.m11 = ca + (1-ca) * rot_x * rot_x;
    rm.m12 = (1-ca) * rot_x * rot_y - sa * rot_z;
    rm.m13 = (1-ca) * rot_z * rot_x + sa * rot_y;
    rm.m21 = (1-ca) * rot_x * rot_y + sa * rot_z;
    rm.m22 = ca + (1-ca) * rot_y * rot_y;
    rm.m23 = (1-ca) * rot_y * rot_z - sa * rot_x;
    rm.m31 = (1-ca) * rot_z * rot_x - sa * rot_y;
    rm.m32 = (1-ca) * rot_y * rot_z + sa * rot_x;
    rm.m33 = ca + (1-ca) * rot_z * rot_z;

    yb_m_mul_v( &rm, vec, &vc );

    *vec = vc;
}


void yb_DoExtraVPHacks( struct ypabact_data *ybd, struct trigger_logic_msg *msg )
{

    /*** Spezieller Hack fuer Plasmabatzen. beisst sich mit Robobeam? ***/
    //if( (ybd->bact.extravp[0].flags & (EVF_Active|EVF_Scale)) &&
    //    (ybd->bact.scale_time > 0) ) {
    if( EVLF_PLASMA == ybd->bact.extravp_logic ) {  
        
        LONG  sc_time = (LONG)( PLASMA_TIME * (FLOAT)ybd->bact.Maximum);
        FLOAT sc_fct  = PLASMA_SCALE;

                if( sc_time < PLASMA_MINTIME )
                        sc_time = PLASMA_MINTIME;
                if( sc_time > PLASMA_MAXTIME )
                        sc_time = PLASMA_MAXTIME;
                        
                yb_DoWhilePlasma( ybd, sc_time, msg->frame_time, sc_fct );
        }

     /*** noch ein hack fuer das Robobeamen ***/
     if( EVLF_BEAM == ybd->bact.extravp_logic ) {

        /*** MUSS EIN ROBO SEIN!!! ***/

        struct ypaworld_data *ywd;
        struct yparobo_data *yrd;

        ywd = INST_DATA( ((struct nucleusdata *)ybd->world)->o_Class, ybd->world);
        yrd = INST_DATA( ((struct nucleusdata *)ybd->bact.BactObject)->o_Class, ybd->bact.BactObject);
        yrd->BeamInTime -= msg->frame_time;

        /*** Ist der BeamIn-Effekt zu Ende? ***/
        if( yrd->BeamInTime <= 0 ) {

            struct flt_triple before_beam;

            yrd->BeamInTime = 0;

            /*** BeamOut ***/
            _StartSoundSource( &(yrd->bact->sc), VP_NOISE_BEAMOUT );

            /*** Move-Zustand abmelden ***/
            yrd->RoboState &= ~ROBO_MOVE;

            /*** VP aus! ***/
            yrd->bact->extravp[ 0 ].flags = 0;
            yrd->bact->extravp[ 1 ].flags = 0;
            }
        else {

            /*** Wir sind noch davor. ***/

            /* --------------------------------------------
            ** Wir lassen um uns einen CreateVP flackern.
            ** Dazu schalten wir ihn abwechselnd an oder 
            ** aus. Je kleiner die Zeit wird, desto laenger
            ** bleibt er an und desto kuerzer werden die
            ** "Ohne"-intervalle.
            ** Ebenso setzen wir was an den Zielpunkt.
            ** ------------------------------------------*/
            if( yrd->beam_fx_time <= 0 ) {

                /*** Es ist an der Zeit, was zu tun ***/
                LONG time_da   = (BEAM_IN_TIME - yrd->BeamInTime) / 10;
                LONG time_wech = yrd->BeamInTime / 10;

                /*** Am alten Ort ***/
                if( yrd->bact->extravp[0].flags & EVF_Active ) {

                    /*** wieder ausschalten ***/
                    yrd->beam_fx_time = time_wech;
                    yrd->bact->extravp[ 0 ].flags &= ~EVF_Active;
                    }
                else {

                    /*** Anschalten ***/
                    yrd->beam_fx_time = time_da;

                    yrd->bact->extravp[ 0 ].pos     = yrd->bact->pos;
                    yrd->bact->extravp[ 0 ].dir     = yrd->bact->dir;
                    yrd->bact->extravp[ 0 ].flags   = (EVF_Active | EVF_Scale);
                    yrd->bact->extravp[ 0 ].scale   = 1.25;
                    yrd->bact->extravp[0].vis_proto = yrd->bact->vis_proto_create;
                    yrd->bact->extravp[0].vp_tform  = yrd->bact->vp_tform_create;
                    }

                /*** Am neuen Ort ***/
                if( yrd->bact->extravp[1].flags & EVF_Active ) {

                    /*** wieder ausschalten ***/
                    yrd->beam_fx_time = time_wech;
                    yrd->bact->extravp[ 1 ].flags &= ~EVF_Active;
                    }
                else {

                    /*** Anschalten ***/
                    yrd->beam_fx_time = time_da;

                    yrd->bact->extravp[ 1 ].pos     = yrd->BeamPos;
                    yrd->bact->extravp[ 1 ].dir     = yrd->bact->dir;
                    yrd->bact->extravp[ 1 ].flags   = EVF_Active;
                    yrd->bact->extravp[1].vis_proto = yrd->bact->vis_proto_create;
                    yrd->bact->extravp[1].vp_tform  = yrd->bact->vp_tform_create;
                    }
               }

            yrd->beam_fx_time -= msg->frame_time;
            }

        }
}

#endif
