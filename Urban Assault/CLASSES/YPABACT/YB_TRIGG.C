/*
**  $Source: PRG:MovingCubesII/Classes/_YPABactClass/yb_trigger.c,v $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:24:14 $
**  $Locker:  $
**  $Author: floh $
**
**  Logic- und Visual-Trigger-Methoden-Handler für ypabact.class.
**
**  (C) Copyright 1995 by A.Weissflog & Andreas Flemming
*/
#include <stdio.h>
#include <stdlib.h>

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/nodes.h>

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "engine/engine.h"
#include "audio/audioengine.h"

#include "ypa/ypaworldclass.h"
#include "ypa/ypabactclass.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine
_extern_use_audio_engine

struct tform GlobalViewerTForm;

void   yb_OutOfWorld_Exception( struct ypabact_data *ybd );
void   yb_UnderEOFException( struct ypabact_data *ybd );
void   yb_RemoveDeadWeapon( struct ypabact_data *ybd );
void   yb_m_rot_round_lokal_x( struct flt_m3x3 *b, FLOAT d );
void   yb_DoBeamStuff( struct ypabact_data *ybd );

/*-----------------------------------------------------------------*/
_dispatcher(void, yb_YBM_TR_LOGIC, struct trigger_logic_msg *msg)
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
    Object *slave, *userrobo;
    struct setstate_msg state;

    /*** Ausfüllen der globalen  Viewer-TForm ***/
    _SetViewer( &GlobalViewerTForm );

    /*** GetSectorInfo-Stuff auf neue Position ***/
    gsi_msg.abspos_x = ybd->bact.pos.x;
    gsi_msg.abspos_z = ybd->bact.pos.z;
    if ( !_methoda(ybd->world, YWM_GETSECTORINFO, &gsi_msg)) {

        /* -------------------------------------------------------------
        ** Wir waren außerhalb der Welt. Ich rufe eine Ausnamebehandlung
        ** auf, die das Objekt wieder korrekt in die Welt setzt, so daß
        ** ein erneuter GETSECTORINFO-Aufruf funktionieren muß!
        ** -----------------------------------------------------------*/

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

    /*** Unter der EOF? Nur bei Extremfällen!!! ***/
    if( ybd->bact.pos.y > ( ybd->bact.Sector->Height + 1000 ) )
        yb_UnderEOFException( ybd );

    /* -----------------------------------------------------------------
    ** BeamSectorTest. Da ist einiges neu: Gebeamt wird erst, wenn der
    ** Robo reingeht. Dann parse ich die BactList und nehme alle, die
    ** nicht auf einem Slurp sind, keine Raketen und nicht ich selbst
    ** bin, mit hinein. Weil man aber was sehen soll, muß das Raussetzen
    ** des Robos verzögert sein.
    ** Dann SaveGame?
    ** ---------------------------------------------------------------*/
    _get( ybd->world, YWA_UserRobo, &userrobo );

    if( (WTYPE_OpenedGate     == ybd->bact.Sector->WType) &&
        (BCLID_YPAROBO        == ybd->bact.BactClassID) &&
        (ybd->bact.BactObject == userrobo) )
        yb_DoBeamStuff( ybd );


    /* -----------------------------------------------------
    ** wenn Energie kleiner als 0, dann DEAD, aber nur, wenn
    ** er vorher i.O. war!
    ** ---------------------------------------------------*/
    if( !(ybd->bact.ExtraState & EXTRA_LOGICDEATH) )
        if( ybd->bact.Energy <= 0 ) {

            /*
            ** Sterben lassen - hier. Doch dies ist nur das logische Sterben.
            ** Wann der brennende VisProto "Dead" zugeschalten wird, entscheidet
            ** die Situation. Dann SetState (!) nutzen, um Explosion anzumelden.
            ** Raketen werden anders verwaltet.
            */
            if( ybd->bact.BactClassID != BCLID_YPAMISSY ) {

                _methoda( o, YBM_DIE, NULL);

                if( !_methoda( o, YBM_TESTDESTROY, NULL) ) {

                    /*** wenigstens wartend (nur VisProto!!!) einstellen ***/
                    state.main_state = ACTION_WAIT;
                    state.extra_off  = state.extra_on = 0;
                    _methoda( o, YBM_SETSTATE, &state );
                    }

                ybd->bact.MainState   = ACTION_DEAD;
                ybd->bact.ExtraState &= ~EXTRA_LANDED;
                }
            }

    /*** Kontrolle an User bzw. AI-Engine ***/
    ybd->bact.internal_time  += msg->frame_time;
    _methoda(o, YBM_AI_LEVEL1, msg);

    /*** Alle Waffen triggern ***/
    waffe = (struct OBNode *) ybd->bact.auto_list.mlh_Head;
    while( waffe->nd.mln_Succ ) {

        struct OBNode *next_waffe = (struct OBNode *) waffe->nd.mln_Succ;
        
        _methoda( waffe->o, YBM_TR_LOGIC, msg );
        waffe = next_waffe;
        }

    /*** tote Waffen raus ***/
    yb_RemoveDeadWeapon( ybd );


    /* ------------------------------------------------------------
    ** nachdem YBM_HANDLEINPUT oder YBM_AI_LEVEL1 jetzt die
    ** Position verändert haben, noch etwas House-Keeping. Wenn wir
    ** einen ExtraViewer haben, dann verschiebt sich alles und der
    ** Viewer wird auch gezeichnet!   
    ** ----------------------------------------------------------*/
    
    if (ybd->flags & YBF_Viewer) {
        
        /*** Die Position ***/
        if( ybd->flags & YBF_ExtraViewer ) {

            GlobalViewerTForm.loc.x = ybd->bact.pos.x +
                                 ybd->bact.dir.m11 * ybd->bact.Viewer.relpos.x +
                                 ybd->bact.dir.m21 * ybd->bact.Viewer.relpos.y +
                                 ybd->bact.dir.m31 * ybd->bact.Viewer.relpos.z;
            GlobalViewerTForm.loc.y = ybd->bact.pos.y +
                                 ybd->bact.dir.m12 * ybd->bact.Viewer.relpos.x +
                                 ybd->bact.dir.m22 * ybd->bact.Viewer.relpos.y +
                                 ybd->bact.dir.m32 * ybd->bact.Viewer.relpos.z;
            GlobalViewerTForm.loc.z = ybd->bact.pos.z +
                                 ybd->bact.dir.m13 * ybd->bact.Viewer.relpos.x +
                                 ybd->bact.dir.m23 * ybd->bact.Viewer.relpos.y +
                                 ybd->bact.dir.m33 * ybd->bact.Viewer.relpos.z;
            }
        else
            GlobalViewerTForm.loc  = ybd->bact.pos;

        /*** Die Matrix ***/
        if( ybd->flags & YBF_ExtraViewer )
            GlobalViewerTForm.loc_m = ybd->bact.Viewer.dir;
        else
            GlobalViewerTForm.loc_m = ybd->bact.dir;
        }

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
        _methoda(slave, YBM_TR_LOGIC, msg);
        msg->slave_count++;

        /*** Nachfolger ***/
        nd = next;
        }
    msg->slave_count = slave_count_store;

    /*** AUDIO: SoundCarrier updaten und refreshen ***/
    ybd->bact.sc.pos   = ybd->bact.pos;
    
    /*** User-geräusch etwas versetzen ***/
    if( ybd->flags & YBF_Viewer ) {

        ybd->bact.sc.pos.x += ybd->bact.dir.m21 * 400;
        ybd->bact.sc.pos.y += ybd->bact.dir.m22 * 400;
        ybd->bact.sc.pos.z += ybd->bact.dir.m23 * 400;
        }

    ybd->bact.sc.vec.x = ybd->bact.dof.x * ybd->bact.dof.v;
    ybd->bact.sc.vec.y = ybd->bact.dof.y * ybd->bact.dof.v;
    ybd->bact.sc.vec.z = ybd->bact.dof.z * ybd->bact.dof.v;
    _RefreshSoundCarrier(&(ybd->bact.sc));

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
_dispatcher(void, yb_YBM_TR_VISUAL, struct basepublish_msg *msg)
/*
**  FUNCTION
**      Wird auf Object angewendet, wenn es sich darstellen soll.
**      Vorher *MUSS* bereits YBM_TR_LOGIC angewendet worden
**      sein!
**
**      Man beachte die "artfremde" basepublish_msg. Diese wird
**      benötigt, weil das Visual Prototype Object ja ein
**      base.class Object ist!
**
**      Diese Msg wird vom Welt-Object ausgefüllt und enthält Pointer
**      auf die globalen Arg- und PubStacks.
**
**  CHANGED
**      09-Jun-95   floh    created
**      11-Jun-95   floh    jetzt mit basepublish_msg
**      11-Oct-95   floh    die msg bekommt jetzt noch die
**                          <owner_id> initialisiert (Wichtig z.B.
**                          für Partikel-Systeme)
*/
{
    struct ypabact_data *ybd = INST_DATA(cl,o);
    int i;

    /*** Wenn Darstellung möglich und gewünscht ***/
    if( ybd->vis_proto && (!(ybd->bact.ExtraState & EXTRA_DONTRENDER)) ) {

        /* -----------------------------------------------------------------
        ** VisProto nur darstellen, wenn ich nicht Viewer bin! Ausnahme sind
        ** die Objekte, bei denen es gefordert wird (z.B. Robos)
        ** ---------------------------------------------------------------*/
        if( (!(ybd->flags & YBF_Viewer)) || (ybd->flags & YBF_AlwaysRender) ) {

            /*** wenn nicht Viewer-Object, dann VisProto ***/
            /*** positionieren und publishen             ***/

            /*** Position und Orientierung in VisProto-TForm kopieren ***/
            ybd->vp_tform->loc   = ybd->bact.tf.loc;
            ybd->vp_tform->loc_m = ybd->bact.tf.loc_m;

            /*** und PUBLISH! ***/
            msg->owner_id = ybd->bact.ident;
            _methoda(ybd->vis_proto, BSM_PUBLISH, msg);
           }
        }
        
    /*** Gibt es alternative Visprotos? ***/
    for( i = 0; i < MAXNUM_EXTRAVP; i++ ) {

        /* -------------------------------------------
        ** DontRender bezieht sich nur auf HauptVP!
                ** Die hier werden ueber ihre Flags geschalten
                ** -----------------------------------------*/
        if( ybd->bact.extravp[i].vis_proto && 
            (ybd->bact.extravp[i].flags & EVF_Active) ) {

            /*** VisProto immer darstellen, weil es ein zusaetzlicher ist ***/
                       
            /*** Position und Orientierung in VisProto-TForm kopieren ***/
            ybd->bact.extravp[i].vp_tform->loc   = ybd->bact.extravp[i].pos;
            if( ybd->bact.extravp[i].flags & EVF_Scale ) {
            
                /* ------------------------------------------------------------
                ** Das Transponieren der Matrix mache ich auch hier gleich mit,
                ** dann brauche ich keinen zusaetzlichen Buffer.
                ** ----------------------------------------------------------*/    
                ybd->bact.extravp[i].vp_tform->loc_m.m11 = ybd->bact.extravp[i].dir.m11 * 
                                                           ybd->bact.extravp[i].scale;
                ybd->bact.extravp[i].vp_tform->loc_m.m12 = ybd->bact.extravp[i].dir.m21 * 
                                                           ybd->bact.extravp[i].scale;
                ybd->bact.extravp[i].vp_tform->loc_m.m13 = ybd->bact.extravp[i].dir.m31 * 
                                                           ybd->bact.extravp[i].scale;
                ybd->bact.extravp[i].vp_tform->loc_m.m21 = ybd->bact.extravp[i].dir.m12 * 
                                                           ybd->bact.extravp[i].scale;
                ybd->bact.extravp[i].vp_tform->loc_m.m22 = ybd->bact.extravp[i].dir.m22 * 
                                                           ybd->bact.extravp[i].scale;
                ybd->bact.extravp[i].vp_tform->loc_m.m23 = ybd->bact.extravp[i].dir.m32 * 
                                                           ybd->bact.extravp[i].scale;
                ybd->bact.extravp[i].vp_tform->loc_m.m31 = ybd->bact.extravp[i].dir.m13 * 
                                                           ybd->bact.extravp[i].scale;
                ybd->bact.extravp[i].vp_tform->loc_m.m32 = ybd->bact.extravp[i].dir.m23 * 
                                                           ybd->bact.extravp[i].scale;
                ybd->bact.extravp[i].vp_tform->loc_m.m33 = ybd->bact.extravp[i].dir.m33 * 
                                                           ybd->bact.extravp[i].scale;
                }
            else {
               
                ybd->bact.extravp[i].vp_tform->loc_m.m11 = ybd->bact.extravp[i].dir.m11;
                ybd->bact.extravp[i].vp_tform->loc_m.m12 = ybd->bact.extravp[i].dir.m21;
                ybd->bact.extravp[i].vp_tform->loc_m.m13 = ybd->bact.extravp[i].dir.m31;
                ybd->bact.extravp[i].vp_tform->loc_m.m21 = ybd->bact.extravp[i].dir.m12;
                ybd->bact.extravp[i].vp_tform->loc_m.m22 = ybd->bact.extravp[i].dir.m22;
                ybd->bact.extravp[i].vp_tform->loc_m.m23 = ybd->bact.extravp[i].dir.m32;
                ybd->bact.extravp[i].vp_tform->loc_m.m31 = ybd->bact.extravp[i].dir.m13;
                ybd->bact.extravp[i].vp_tform->loc_m.m32 = ybd->bact.extravp[i].dir.m23;
                ybd->bact.extravp[i].vp_tform->loc_m.m33 = ybd->bact.extravp[i].dir.m33;
                }
                
            /*** und PUBLISH! ***/
            msg->owner_id = ybd->bact.ident;
            _methoda(ybd->bact.extravp[i].vis_proto, BSM_PUBLISH, msg);
            }
        }    
    
        
           
    /*** das war's bereits ***/
}



void yb_OutOfWorld_Exception( struct ypabact_data *ybd )
{
    /*
    ** Manchmal geschieht es doch, daß das Objekt außerhalb der Welt
    ** ist. Wir setzen es dann ganz einfach auf die nächstmögliche
    ** Position in die Welt, auch auf die gefahr hin, daß es dann in einem
    ** Gebäude ist....
    */

    FLOAT max_world_x, max_world_z;
    ULONG sec_x, sec_y;

    _get(ybd->world, YWA_MapSizeX, &sec_x );
    _get(ybd->world, YWA_MapSizeY, &sec_y );

    max_world_x = (FLOAT) (sec_x * SECTOR_SIZE);
    max_world_z = (FLOAT) (sec_y * SECTOR_SIZE);

    if( ybd->bact.pos.x > max_world_x ) 
        ybd->bact.pos.x = max_world_x - SECTOR_SIZE / 2;

    if( ybd->bact.pos.x < 0.0 )
        ybd->bact.pos.x = SECTOR_SIZE / 2;

    /* z-Werte sind negativ! */

    if( ybd->bact.pos.z < -max_world_z )
        ybd->bact.pos.z = -max_world_z + SECTOR_SIZE / 2;

    if( ybd->bact.pos.z > 0.0 )
        ybd->bact.pos.z = -(SECTOR_SIZE / 2);

    yb_UnderEOFException( ybd );

}


void yb_UnderEOFException( struct ypabact_data *ybd )
{
    /*
    ** Es kann passieren, daß wir unter die EOF kommen. Passiert eben.
    ** Wir suchen dann den höchsten Punkt, den wir finden müssten, weil
    ** vorher ein OutOfWorldtest war...
    ** Später mal durch 'ne Art GetHeight ersetzen!
    */

    struct intersect_msg inter;
//_LogMsg("UnderEOFException at Sector x %d y %d (%7.2f,%9.2f)   of class %d   and type %d\n",
//                 ybd->bact.SectX, ybd->bact.SectY, ybd->bact.pos.x,
//                 ybd->bact.pos.z, ybd->bact.BactClassID, ybd->bact.TypeID);

    inter.pnt.x = ybd->bact.pos.x;
    inter.pnt.y = -30000.0;
    inter.pnt.z = ybd->bact.pos.z;
    inter.vec.x = 0;
    inter.vec.z = 0;
    inter.vec.y = 50000;
    inter.flags = 0;

    _methoda( ybd->world, YWM_INTERSECT, &inter );
    if( inter.insect ) {

        /*** Was passieren sollte!!! ***/
        ybd->bact.pos.y = inter.ipnt.y - 50;
        }
    else {

        /*** Hilfsweise Sektor benutzen ***/
        ybd->bact.pos.y = ybd->bact.Sector->Height - 50;
        }

}

void yb_RemoveDeadWeapon( struct ypabact_data *ybd )
{
    /* ---------------------------------------------------------------
    ** Entfernt alle Waffen, die wir nicht mehr triggern müssen und
    ** gibt sie in den Totenpool zurück. Das müssen wir so umständlich
    ** machen, weil Waffen in einer anderen Liste sind.
    ** -------------------------------------------------------------*/

    struct OBNode *node, *next_node;
    struct settarget_msg st;
    LONG   YLS;
    
    node = (struct OBNode *) ybd->bact.auto_list.mlh_Head;

    while( node->nd.mln_Succ ) {

        next_node = (struct OBNode *) node->nd.mln_Succ;

        /*** testen, ob sie schon (zeitlich) weit runter ist ***/
        _get( node->o, YBA_YourLS, &YLS );
        if( YLS <= 0 ) {

            /* ---------------------------------------------------------
            ** Wir hatten vielleicht ein Ziel, wo wir Attacker waren und
            ** welches noch nicht tot ist 
            ** -------------------------------------------------------*/
            st.target_type = TARTYPE_NONE;
            st.priority    = 0;
            _methoda( node->o, YBM_SETTARGET, &st );
            
            _Remove( (struct Node *) node );

            /*** jetzt bekommt es die Welt, der wir vortäuschen, es wäre neu ***/
            node->bact->master = NULL;
            _methoda( ybd->bact.BactObject, YBM_RELEASEVEHICLE, node->o );
            }

        node = next_node;
        }
}



_dispatcher( void, yb_YBM_SETFOOTPRINT, void *nix )
{
    /*
    ** Wenn der Eigentümer unter 8 ist, dann kann er ein Bit in FootPrint
    ** hinterlassen. Damit ist das Gebiet dem zugehörigen Robo bekannt.
    ** Der radius des gebietes, in dem wir unseren Abdruck hinterlassen,
    ** steht in bact->View, wobei 0 nur eigener Sektor bedeutet.
    */

    struct Cell *sector;
    int    ofx, ofy, myx, myy, view;
    struct ypabact_data *ybd;
    struct OBNode *Slave;

    ybd = INST_DATA( cl, o);

    if( ybd->bact.BactClassID == BCLID_YPAMISSY ) return;

    /*** Mir footprinten nich ***/
    if( ( ybd->bact.MainState == ACTION_DEAD ) ||
        ( ybd->bact.MainState == ACTION_CREATE ) ) return;


    /* -------------------------------------------------------------
    ** Das ganze hat nur Sinn, wenn für mich noch niemand die Arbeit
    ** erledigt hat. Vgl. dazu FootPrint und Sektor-Pos
    ** -----------------------------------------------------------*/
    if( (ybd->bact.master_bact == NULL) ||
        ((ybd->bact.master_bact != NULL) &&
        (!( (ybd->bact.master_bact->View  >= ybd->bact.View) &&
            (ybd->bact.master_bact->SectX == ybd->bact.SectX) &&
            (ybd->bact.master_bact->SectY == ybd->bact.SectY) ))) ) {

        /*** Mein Footprint ***/
        if( ybd->bact.Owner < 8 ) {

            UBYTE mask = 1 << ybd->bact.Owner;
            
            myx    = ybd->bact.SectX;
            myy    = ybd->bact.SectY;
            view   = ybd->bact.View;
            sector = ybd->bact.Sector;

            for( ofy = -view; ofy <= view; ofy++ ) {

                /* --------------------------------------------------------------
                ** Nun auf dieser Achse alle y-Werte, die extra berechnet werden,
                ** damit wir einen Kreis bekommen, für den häufig auftretenden 
                ** Sonderfall View == 1 machen wir eine Sonderbehandlung, die 
                ** a) schneller ist und
                ** b) ein Quadrat zeichnet, was das Anschleichen über Ecken
                **    verhindert
                ** ------------------------------------------------------------*/
                int offset = ofy * ybd->bact.WSecX;

                if( view == 1 ) {

                    /*** test notwendig, weil Objekte auf 0,0 stehen können ***/
                    if( ((myy + ofy) > 0) &&
                        ((myy + ofy) < (ybd->bact.WSecY-1)) ) {

                        /*** Die zeile liegt im Spiel ***/
                        if( (myx - 1) > 0 )
                            sector[ offset - 1 ].FootPrint |= mask;

                        sector[ offset     ].FootPrint |= mask;

                        if( (myx + 1) < (ybd->bact.WSecX-1) )
                            sector[ offset + 1 ].FootPrint |= mask;
                        }
                    }
                else {
                    int rand = FLOAT_TO_INT( nc_sqrt( (FLOAT)( view*view - ofy*ofy )));

                    for( ofx =  -rand; ofx <=  rand; ofx++ ) {

                        int offset2;

                        /*** testen, ob in der Welt ***/
                        if( ((myx + ofx) > 0) &&
                            ((myx + ofx) < (ybd->bact.WSecX-1)) &&
                            ((myy + ofy) > 0) &&
                            ((myy + ofy) < (ybd->bact.WSecY-1))) {

                            /*** Abdruck ***/
                            offset2 = offset + ofx;
                            sector[ offset2 ].FootPrint |= mask;
                            }
                        }
                    }
                }
            }
        }

    /*** Alle Untergebenen ***/
    Slave = (struct OBNode *) ybd->bact.slave_list.mlh_Head;
    while( Slave->nd.mln_Succ ) {

        _methoda( Slave->o, YBM_SETFOOTPRINT, NULL);
        Slave = (struct OBNode *) Slave->nd.mln_Succ;
        }
}


void yb_DoBeamStuff( struct ypabact_data *ybd )
{
    struct ypaworld_data *ywd;
    ywd = INST_DATA( ((struct nucleusdata *)ybd->world)->o_Class, ybd->world);

    /*** Bactlist schon gescannt? ***/
    if( ybd->bact.startbeam != 0 ) {

        /*** ist genügend Zeit vergangen? ***/
        if( ybd->bact.internal_time - ybd->bact.startbeam > 2000 ) {

            /*** Sind noch "BEAM"-Vehicle da? dann warten... ***/
            struct Bacterium *bact;
            BOOL   bf = FALSE;

            bact = (struct Bacterium *) ybd->bact.Sector->BactList.mlh_Head;
            while( bact->SectorNode.mln_Succ ) {

                ULONG viewer;

                /*** Der Viewer muss nicht verschwunden sein... ***/
                _get( bact->BactObject, YBA_Viewer, &viewer );

                if( (ACTION_BEAM     == bact->MainState) &&
                    (ybd->bact.Owner == bact->Owner) &&
                    (viewer          == FALSE) )
                    bf = TRUE;
                bact = (struct Bacterium *) bact->SectorNode.mln_Succ;
                }

            if( !bf ) {


                /*** Ok, das Spiel abbrechen ***/
                struct beamnotify_msg bnm;
                char   filename[ 200 ];
                struct flt_triple merke_pos;
                struct saveloadgame_msg slg;
                struct yparobo_data *yrd = INST_DATA(
                       ((struct nucleusdata *)ybd->bact.BactObject)->o_Class,
                       ybd->bact.BactObject);

                bnm.b = &(ybd->bact);
                _methoda(ybd->world, YWM_BEAMNOTIFY, &bnm);

                /* --------------------------------------------------------
                ** Abspeichern des restes, bevor alles gelöscht wird. Beim
                ** Wiedereintritt sind wir auf der Position vor dem Beamen.
                ** Dazu muß auch merke_y korrigiert werden!
                ** ------------------------------------------------------*/
                merke_pos     = ybd->bact.pos;
                ybd->bact.pos = ybd->bact.old_pos;
                yrd->merke_y  = ybd->bact.old_pos.y;
                slg.gsr       = ywd->gsr;
                slg.name      = filename;
                sprintf( filename, "save:%s/%d.fin\0",
                                   ywd->gsr->UserName,
                                   ywd->Level->Num );
                if( !_methoda( ybd->world, YWM_SAVEGAME, &slg ) )
                    _LogMsg("Warning, Save error\n");
                else {
                    /*** nun ist rst-File nicht mehr notwendig ***/
                    sprintf( filename, "save:%s/%d.rst\0",
                                       ywd->gsr->UserName,
                                       ywd->Level->Num );
                    _FDelete( filename );
                    }
    
                ybd->bact.pos = merke_pos;

                /* --------------------------------------------
                ** Solange es keine neue Strategie gibt, Slaves
                ** töten. Neu: Wenn vorher EXTRA_CLEANUP, dann
                ** kein normaler Tod, sondern Aufräumaktion.
                ** meldungen werden unterdrückt.
                ** ------------------------------------------*/
                ybd->bact.ExtraState |= EXTRA_CLEANUP;
                _methoda( ybd->bact.BactObject, YBM_DIE, NULL );
                }
            }
        }
    else {

        struct Bacterium *kandidat;
        struct OBNode *com;
        LONG   count = 0;

        /* ---------------------------------------------------------
        ** Zuerst positioniere ich mich auf ie Mitte des Sektors, um
        ** keine probleme mit der Beamgate-Architektur zu bekommen
        ** -------------------------------------------------------*/
        ybd->bact.pos.x =  ( ((FLOAT)ybd->bact.SectX)+ 0.5 ) * SECTOR_SIZE;
        ybd->bact.pos.z = -( ((FLOAT)ybd->bact.SectY)+ 0.5 ) * SECTOR_SIZE;

        /* ---------------------------------------------------------------
        ** Dann setze ich bei ALLEN meinen Untergebenen die CLEANUP-Flags,
        ** weil keiner Meldung machen darf. In den BeamZustand kommen
        ** aber nur die im Sektor!
        ** -------------------------------------------------------------*/
        com = (struct OBNode *) ybd->bact.slave_list.mlh_Head;
        while( com->nd.mln_Succ ) {

            struct OBNode *slave;

            com->bact->ExtraState |= EXTRA_CLEANUP;
            slave = (struct OBNode *) com->bact->slave_list.mlh_Head;
            while( slave->nd.mln_Succ ) {

                slave->bact->ExtraState |= EXTRA_CLEANUP;
                slave = (struct OBNode *) slave->nd.mln_Succ;
                }
            com = (struct OBNode *) com->nd.mln_Succ;
            }

        /* -----------------------------------------------------------
        ** BactList scannen. Ich ignoriere mich, Raketen, Feinde
        ** und Tote. All diese setze ich auf beam mit einer Eingangs-
        ** zeit.
        ** Die startbeam-zeit beim Robo sagt, daß es schon getan wurde
        ** ---------------------------------------------------------*/
        kandidat = (struct Bacterium *) ybd->bact.Sector->BactList.mlh_Head;

        /*** Buddie-Array löschen ***/
        ywd->Level->NumBuddies = 0;
        memset(ywd->Level->Buddies,0,sizeof(ywd->Level->Buddies));

        while( kandidat->SectorNode.mln_Succ ) {

            FLOAT  dx, dz;

            /*** Was für uns? ***/
            if( (ACTION_DEAD     == kandidat->MainState) ||
                (BCLID_YPAROBO   == kandidat->BactClassID) ||
                (BCLID_YPAMISSY  == kandidat->BactClassID) ||
                (BCLID_YPAGUN    == kandidat->BactClassID) ||
                (ybd->bact.Owner != kandidat->Owner) ) {

                kandidat = (struct Bacterium *) kandidat->SectorNode.mln_Succ;
                continue;
                }

            /*** Ist das auf dem sektor? ***/
            dx = kandidat->pos.x - (ybd->bact.SectX + 0.5) * SECTOR_SIZE;
            dz = kandidat->pos.z + (ybd->bact.SectY + 0.5) * SECTOR_SIZE;
            if( ( fabs( dx ) > 3 * SECTOR_SIZE / 8 ) &&
                ( fabs( dz ) > 3 * SECTOR_SIZE / 8 ) ) {

                kandidat = (struct Bacterium *) kandidat->SectorNode.mln_Succ;
                continue;
                }

            /*** In BeamStatus setzen ***/
            kandidat->scale_time  = 0;
            kandidat->scale_delay = count++ * 400 + 1000;

            /*** Visproto noch nicht umsetzen ***/
            kandidat->MainState = ACTION_BEAM;

            /*** next one ***/
            kandidat = (struct Bacterium *) kandidat->SectorNode.mln_Succ;
            }

        ybd->bact.startbeam = ybd->bact.internal_time;
        }
}


void yb_m_rot_round_lokal_x( struct flt_m3x3 *m, FLOAT angle)
{

    struct flt_m3x3 neu_dir, rm;
    FLOAT sin_y = sin( angle );
    FLOAT cos_y = cos( angle );


    rm.m11 = 1.0;       rm.m12 = 0.0;       rm.m13 = 0.0;
    rm.m21 = 0.0;       rm.m22 = cos_y;     rm.m23 = sin_y;
    rm.m31 = 0.0;       rm.m32 = -sin_y;    rm.m33 = cos_y;

    nc_m_mul_m(&rm, m, &neu_dir);

    *m = neu_dir;
}



