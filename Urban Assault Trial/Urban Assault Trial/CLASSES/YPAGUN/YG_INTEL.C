/*
**  $Source: $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:24:47 $
**  $Locker: $
**  $Author: $
**
**  Die Intelligenzen...
**
**  (C) Copyright 1995 by Andreas Flemming
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


/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine


/*-----------------------------------------------------------------*/
void yg_m_mul_v(struct flt_m3x3 *m1, struct flt_triple *v1, struct flt_triple *v2);
void yg_vec_round_vec( struct flt_triple *rot, struct flt_triple *vec, FLOAT angle);
void yg_dir_round_vec( struct flt_triple *rot, struct flt_m3x3 *dir, FLOAT angle );
void yg_RotLokalY( struct ypagun_data *ygd, struct flt_triple *new_y, FLOAT time);
void yg_rot_round_lokal_x( struct Bacterium *bact, FLOAT angle);
void yg_rot_round_rot( struct ypagun_data *gun, FLOAT angle);
BOOL yg_CheckGround( struct ypagun_data *ygd );

/*-----------------------------------------------------------------*/


_dispatcher(void, yg_YBM_AI_LEVEL3, struct trigger_logic_msg *msg )
/*
**  FUNCTION
**
**      Richtet das Objekt mit seiner lokalen z-Achse auf ein Ziel aus, welches
**      ein Bakterien-Nebenziel ist. Alles andere hat für uns keine Bedeutung
**      und muß ignoriert werden.
**
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      28-Jun-95   8100 000C    created
*/
{
    
    struct ypagun_data *ygd = INST_DATA(cl,o);
    FLOAT  y_angle, y_rotate, y_wish;
    FLOAT  xz_rotate, xz_wish, xz_angle;
    struct flt_triple richtung, norm, lx;
    FLOAT  distance, time;
    struct settarget_msg target;
    struct fight_msg fight;
    struct setstate_msg state;

    time = (FLOAT) msg->frame_time / 1000.0;

    switch( ygd->bact->MainState ) {

        case ACTION_NORMAL:
        case ACTION_WAIT:

            /* -----------------------------------------------------------------
            ** Falltest. Wenn wir keine Bordkanone sind, dann haben wir eine
            ** Aufhängung bzw. einen Untergrund. Wenn dieser nicht mehr gefunden 
            ** wird, dann stellen wir uns energetisch auf unter 0 und den Rest 
            ** erledigt DIE und ACTION_DEAD automatisch.
            ** ---------------------------------------------------------------*/
            if( !(ygd->flags & GUN_RoboGun) ) {

                if( (ygd->bact->internal_time - ygd->ground_time) > 800 ) {

                    ygd->ground_time = ygd->bact->internal_time;

                    if( !yg_CheckGround( ygd ) ) {

                        ygd->bact->Energy = -10;
                        break;
                        }
                    }
                }

            /*** Zeit runterzählen ***/
            if( ygd->firetype == GF_Real ) {

                if( ygd->firecount > 0 ) {

                    ygd->firecount -= msg->frame_time;
                    if( ygd->firecount <= 0 ) {

                        ygd->firecount = 0;
                        state.extra_off = EXTRA_FIRE;
                        state.extra_on  = state.main_state = 0;
                        _methoda( o, YBM_SETSTATE, &state );
                        }
                    }
                }


            /* ----------------------------------------------------------------------
            ** In Flaks kann durch AI1/2 vielleicht WAIt auftauchen, das hat aber
            ** keinen Sinn. Deshalb hier beides. Das dazu. Jetzt ermittle ich erstmal
            ** den Vektor auf das Ziel.
            ** --------------------------------------------------------------------*/

            if( ygd->bact->SecTargetType == TARTYPE_BACTERIUM ) {

                /*** Das Bacterium ist aktuell! ***/
                richtung.x = ygd->bact->SecondaryTarget.Bact->pos.x - ygd->bact->pos.x;
                richtung.y = ygd->bact->SecondaryTarget.Bact->pos.y - ygd->bact->pos.y;
                richtung.z = ygd->bact->SecondaryTarget.Bact->pos.z - ygd->bact->pos.z;
                
                distance = nc_sqrt( richtung.x * richtung.x + richtung.y * richtung.y +
                                    richtung.z * richtung.z );

                if( distance > 0.001 ) {

                    richtung.x /= distance;
                    richtung.y /= distance;
                    richtung.z /= distance;
                    }
                else return;

                /* -------------------------------------------------------------
                ** Jetzt haben wir Richtung und Entfernung zum Ziel. Richten wir
                ** uns doch mal darauf aus. Dabei müssen wir beachten, daß der
                ** Basisvektor mit den Maximalwinkeln eine Beschränkung vorgibt.
                ** -----------------------------------------------------------*/
                
                /*--------------------------------------------------------------
                **    Seitendrehung
                **------------------------------------------------------------*/

                /* -----------------------------------------------------------
                ** Der Winkel, auf den ich mich um die rotachse drehen muß,
                ** und auch der Winkel, um den ich mich gedreht habe,
                ** ist der Winkel zwischen dem Vektor und der rot-basis-Ebene.
                ** Allerdings bekomme ich nur 90-Grad-Winkel (allerdings
                ** mit Vorzeichen). Um zu testen, ob der Betrag des Winkels
                ** größer 90 Grad ist, nutzen wir das Skalarprodukt
                ** ---------------------------------------------------------*/

                norm.x = ygd->basis.y * ygd->rot.z - ygd->basis.z * ygd->rot.y;
                norm.y = ygd->basis.z * ygd->rot.x - ygd->basis.x * ygd->rot.z;
                norm.z = ygd->basis.x * ygd->rot.y - ygd->basis.y * ygd->rot.x;

                lx.x = ygd->rot.y * ygd->basis.z - ygd->rot.z * ygd->basis.y;
                lx.y = ygd->rot.z * ygd->basis.x - ygd->rot.x * ygd->basis.z;
                lx.z = ygd->rot.x * ygd->basis.y - ygd->rot.y * ygd->basis.x;

                xz_angle = nc_acos( lx.x * ygd->bact->dir.m11 +
                                    lx.y * ygd->bact->dir.m12 +
                                    lx.z * ygd->bact->dir.m13 );

                if( (lx.x * ygd->bact->dir.m13 - lx.z * ygd->bact->dir.m11) < 0 )
                    xz_angle = -xz_angle;

                xz_wish = nc_asin( norm.x * richtung.x + norm.y * richtung.y +
                                   norm.z * richtung.z );

                if( nc_acos( ygd->basis.x * richtung.x +
                             ygd->basis.y * richtung.y +
                             ygd->basis.z * richtung.z ) > (PI / 2) ) {

                    /*** Nachkorrigieren ***/
                    if( xz_wish < 0 )  xz_wish = -PI - xz_wish;
                    if( xz_wish > 0 )  xz_wish =  PI - xz_wish;
                    }

                /*** Seitenbegrenzung ***/
                if( ygd->max_xz <= 3.1 ) {

                    if( xz_wish < -ygd->max_xz ) xz_wish = -ygd->max_xz;
                    if( xz_wish >  ygd->max_xz ) xz_wish =  ygd->max_xz;
                    }

                xz_rotate = xz_wish - xz_angle;

                /*** Bei Rundum-drehungen kann ich die Abkürzung nehmen ***/
                if( (ygd->max_xz > 3.1) && ( fabs(xz_rotate) > PI) ) {

                    if( xz_rotate < -PI ) xz_rotate =  2 * PI + xz_rotate;
                    if( xz_rotate >  PI ) xz_rotate = -2 * PI + xz_rotate;
                    }
                    
                if( xz_rotate < 0.0 ) {
                    if( xz_rotate < -ygd->bact->max_rot * time )
                        xz_rotate = -ygd->bact->max_rot * time;
                    }
                else {
                    if( xz_rotate >  ygd->bact->max_rot * time )
                        xz_rotate =  ygd->bact->max_rot * time;
                    }

                if( (xz_rotate < -0.001) || (xz_rotate > 0.001) ) {

                    /*** Es lohnt zu rotieren ***/
                    xz_rotate = -xz_rotate;
                    yg_rot_round_rot( ygd, xz_rotate );
                    }
                

                /*---------------------------------------------------------
                **  Höhendrehung
                **-------------------------------------------------------*/

                y_angle = nc_asin( ygd->bact->dir.m31 * (-ygd->rot.x) +
                                   ygd->bact->dir.m32 * (-ygd->rot.y) +
                                   ygd->bact->dir.m33 * (-ygd->rot.z) );

                y_wish  = nc_asin( richtung.x * (-ygd->rot.x) +
                                   richtung.y * (-ygd->rot.y) +
                                   richtung.z * (-ygd->rot.z) );


                if( y_wish >  ygd->max_up )    y_wish =  ygd->max_up;
                if( y_wish < -ygd->max_down )  y_wish = -ygd->max_down;

                y_rotate = y_wish - y_angle;

                if( y_rotate < 0.0 ) {
                    if( y_rotate < -ygd->bact->max_rot * time )
                        y_rotate = -ygd->bact->max_rot * time;
                    }
                else {
                    if( y_rotate >  ygd->bact->max_rot * time )
                        y_rotate =  ygd->bact->max_rot * time;
                    }

                if( (y_rotate < -0.001) || (y_rotate > 0.001) ) {

                    /*** Es lohnt zu rotieren ***/
                    y_rotate *= 0.3;
                    yg_rot_round_lokal_x( ygd->bact, y_rotate );
                    }


                /*------------------------------------------------------------
                **    Schießen
                **----------------------------------------------------------*/

                fight.time        = time;
                fight.global_time = ygd->bact->internal_time;
                fight.enemy.bact  = ygd->bact->SecondaryTarget.Bact;
                _methoda( o, YBM_FIGHTBACT, &fight );


                /* ---------------------------------------------------------
                ** Ziele abmelden. Wir können Ziele nicht verfolgen, müssen
                ** also selbst gucken, ob sie in (möglicher) Schußposition 
                ** sind. Wenn nicht, dann sofort abmelden. Wir nehmen hierzu 
                ** YBM_TESTECTARGET, welches auch bei der Nebenzielaufnahme 
                ** verwendet wird.
                ** -------------------------------------------------------*/
                if( !_methoda( o, YBM_TESTSECTARGET, 
                      ygd->bact->SecondaryTarget.Bact) ) {

                    target.priority    = 1;
                    target.target_type = TARTYPE_NONE;
                    _methoda( o, YBM_SETTARGET, &target );

                    /*** Falls wir ein MG sind, VisProto ausschalten ***/
                    if( GF_Proto == ygd->firetype ) {

                        state.extra_off = EXTRA_FIRE;
                        state.extra_on  = state.main_state = 0;
                        _methoda( o, YBM_SETSTATE, &state );
                        }
                    }
                }
            else {

                if( TARTYPE_NONE == ygd->bact->SecTargetType ) {

                    /* -----------------------------------------------
                    ** Es kann sein, daß das Ziel abgemeldet wurde und
                    ** ich noch feuere. Dann schalte ich aus
                    ** ---------------------------------------------*/
                    if( (GF_Proto == ygd->firetype) &&
                        (ygd->bact->ExtraState & EXTRA_FIRE) ) {

                        state.extra_off = EXTRA_FIRE;
                        state.extra_on  = state.main_state = 0;
                        _methoda( o, YBM_SETSTATE, &state );
                        }
                    }
                }

            break;

        case ACTION_DEAD:

            _methoda( o, YBM_DOWHILEDEATH, msg );
            break;

        case ACTION_CREATE:

            ygd->bact->scale_time -= msg->frame_time;
            if( ygd->bact->scale_time <= 0 ) {

                /*** Wieder normal werden ***/
                state.main_state = ACTION_NORMAL;
                state.extra_off  = state.extra_on = 0;
                _methoda( o, YBM_SETSTATE, &state );

                /*** Position nicht verändern ***/
                }
            else {

                /*** nichts  machen. Wegen INSTALLGUN! ***/
                }
            break;
        }

    return;
    
}






_dispatcher(void, yg_YBM_HANDLEINPUT, struct trigger_logic_msg *msg)
/*
**  FUNCTION
**
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      28-Jun-95   8100 000C    created
*/
{
    
    FLOAT  dist, xz_angle, xz_rotate, y_angle, y_rotate, time;
    struct flt_triple lx, *relpos;
    struct firemissile_msg fire;
    struct ypagun_data *ygd = INST_DATA(cl,o);
    struct fireyourguns_msg fyg;
    struct setstate_msg state;
    struct visier_msg visier;
    struct crash_msg crash;

    time = (FLOAT) msg->frame_time / 1000.0;

    switch( ygd->bact->MainState ) {

        case ACTION_NORMAL:
        case ACTION_WAIT:

            /* -----------------------------------------------------------------
            ** Falltest. Wenn wir keine Bordkanone sind, dann haben wir eine
            ** Aufhängung bzw. einen Untergrund. Wenn dieser nicht mehr gefunden 
            ** wird, dann stellen wir uns energetisch auf unter 0 und den Rest 
            ** erledigt DIE und ACTION_DEAD automatisch.
            ** ---------------------------------------------------------------*/
            if( !(ygd->flags & GUN_RoboGun) )
                if( !yg_CheckGround( ygd ) ) {

                    ygd->bact->Energy = -10;
                    break;
                    }

            /*** Rückstoß abarbeiten ***/
            relpos = &(ygd->bact->Viewer.relpos);

            dist = nc_sqrt( (relpos->x) * (relpos->x) +
                            (relpos->y) * (relpos->y) +
                            (relpos->z) * (relpos->z) );
            
            if( dist < 3.0 ) {

                relpos->x = relpos->y = relpos->z = 0.0;
                }
            else {

                /*** Halbierung aller 50ms ***/
                LONG halftime = 50;
                relpos->x = (relpos->x) * halftime / (halftime + msg->frame_time);
                relpos->y = (relpos->y) * halftime / (halftime + msg->frame_time);
                relpos->z = (relpos->z) * halftime / (halftime + msg->frame_time);
                }

            /*** Feuern evtl. Ausschalten ***/
            if( (ygd->bact->ExtraState & EXTRA_FIRE) &&
                (!(msg->input->Buttons & BT_FIRE)) &&
                (!(msg->input->Buttons & BT_FIREVIEW)) ) {

                /*** Feuern aus ***/
                // Abmelden beim WO

                state.main_state = state.extra_on = 0;
                state.extra_off  = EXTRA_FIRE;
                _methoda( o, YBM_SETSTATE, &state );
                }

            /* ---------------------
            ** Visier immer zeichnen 
            ** -------------------*/

            /*** Zielaufnahme, die machen wir immer (vorher default) ***/
            fire.target_type  = TARTYPE_SECTOR;
            fire.target_pos.x = ygd->bact->pos.x +
                                ygd->bact->dir.m31 * SECTOR_SIZE * 3;
            fire.target_pos.y = ygd->bact->pos.y +
                                ygd->bact->dir.m32 * SECTOR_SIZE * 3;
            fire.target_pos.z = ygd->bact->pos.z +
                                ygd->bact->dir.m33 * SECTOR_SIZE * 3;
            visier.dir.x = ygd->bact->dir.m31;
            visier.dir.y = ygd->bact->dir.m32;
            visier.dir.z = ygd->bact->dir.m33;
            visier.flags = VISIER_ENEMY | VISIER_NEUTRAL;
            if( _methoda( o, YBM_VISIER, &visier ) ) {

                fire.target_type = TARTYPE_BACTERIUM;
                fire.target.bact = visier.enemy;
                }

            /*** Schießen zuerst, weil das Bewegung beeinflussen kann ***/
            if( (msg->input->Buttons & BT_FIRE) ||
                (msg->input->Buttons & BT_FIREVIEW) ) {

                switch( ygd->firetype ) {

                    case GF_Real:


                        fire.wtype       = ygd->bact->auto_ID;
                        fire.dir.x       = ygd->bact->dir.m31;
                        fire.dir.y       = ygd->bact->dir.m32;
                        fire.dir.z       = ygd->bact->dir.m33;
                        fire.global_time = ygd->bact->internal_time;
                        
                        /*** Gun. etwas höher ***/
                        fire.start.x     = ygd->bact->firepos.x;
                        fire.start.y     = ygd->bact->firepos.y;
                        fire.start.z     = ygd->bact->firepos.z;
                        /*** Viewer? ***/
                        if( msg->input->Buttons & BT_FIREVIEW )
                            fire.flags   = FIRE_VIEW;
                        else
                            fire.flags   = 0;
                        fire.flags      |= FIRE_CORRECT;
                        if( _methoda( o, YBM_FIREMISSILE, &fire ) ) {

                            /*** Rückstoß nur, wenn auch Schuß ***/
                            relpos->x = 0.0;
                            relpos->y = 0.0;
                            relpos->z = -25;
                            }

                        break;

                    case GF_Proto:

                        /*** Schießen, Kanone ***/

                        /*** Feuern einschalten? ***/
                        if( ~(ygd->bact->ExtraState & EXTRA_FIRE) ) {

                            // Anmelden WO

                            state.main_state = state.extra_off = 0;
                            state.extra_on   = EXTRA_FIRE;
                            _methoda( o, YBM_SETSTATE, &state );
                            }

                        /*** Feuern an sich ***/
                        fyg.dir.x = ygd->bact->dir.m31;
                        fyg.dir.y = ygd->bact->dir.m32;
                        fyg.dir.z = ygd->bact->dir.m33;
                        fyg.time  = time;
                        fyg.global_time = ygd->bact->internal_time;
                        _methoda( o, YBM_FIREYOURGUNS, &fyg );

                        /* ---------------------------------------------------
                        ** Shakes und verschiebungen gibt es nicht Schußweise,
                        ** soondern permanent, auf lahmen Hütten gehen die MGs
                        ** eben etwas langsamer
                        ** Übrigens: Metrowerks wollen ihren C++ Compiler an
                        ** das OOP-Modell der BeBox (ist das IBM-Zeug) an-
                        ** passen. Damit sind OS-OOP und C++ nicht mehr
                        ** getrennt. Ich find das so geil!
                        ** -------------------------------------------------*/

                        

                        if( ygd->flags & GUN_Shot ) {

                            ygd->flags &= ~GUN_Shot;
                            }
                        else {

                            ygd->flags |=  GUN_Shot;

                            relpos->x  = 0.0;
                            relpos->y  = -20;
                            relpos->z  = -30;
                            }
                        break;

                    default:

                        /*** nüscht ***/
                        break;
                    }
                }

            /*** Seitendrehung? ***/
            xz_rotate = msg->input->Slider[ SL_FLY_DIR ] *
                        ygd->bact->max_rot * time;

            if( fabs( xz_rotate ) > 0.001 ) {

                lx.x = ygd->rot.y * ygd->basis.z - ygd->rot.z * ygd->basis.y;
                lx.y = ygd->rot.z * ygd->basis.x - ygd->rot.x * ygd->basis.z;
                lx.z = ygd->rot.x * ygd->basis.y - ygd->rot.y * ygd->basis.x;

                xz_angle = nc_acos( lx.x * ygd->bact->dir.m11 +
                                    lx.y * ygd->bact->dir.m12 +
                                    lx.z * ygd->bact->dir.m13 );

                /*** zur Zeit HI und AI hier untersch. Vorzeichen !? ***/
                if( (lx.x * ygd->bact->dir.m13 - lx.z * ygd->bact->dir.m11) > 0 )
                    xz_angle = -xz_angle;

                /*** Begrenzung ***/
                if( (xz_angle + xz_rotate) < -ygd->max_xz )
                    xz_rotate = -ygd->max_xz - xz_angle;
                if( (xz_angle + xz_rotate) >  ygd->max_xz )
                    xz_rotate =  ygd->max_xz - xz_angle;

                /*** Drehen ***/
                yg_rot_round_rot( ygd, xz_rotate );
                }

            /*** Kanone rauf bzw. runter ***/
            y_rotate = msg->input->Slider[ SL_FLY_HEIGHT ] *
                       ygd->bact->max_rot * time;
            
            if( fabs( y_rotate ) > 0.001 ) {
                
                y_angle = nc_asin( ygd->bact->dir.m31 * (-ygd->rot.x) +
                                   ygd->bact->dir.m32 * (-ygd->rot.y) +
                                   ygd->bact->dir.m33 * (-ygd->rot.z) );

                /*** Begrenzung ***/
                if( (y_angle + y_rotate) < -ygd->max_down )
                    y_rotate = -ygd->max_down - y_angle;
                if( (y_angle + y_rotate) >  ygd->max_up )
                    y_rotate =  ygd->max_up - y_angle;

                yg_rot_round_lokal_x( ygd->bact, y_rotate );
                }

            /*** neue Matrix in ExtraViewer übernehmen ***/
            ygd->bact->Viewer.dir = ygd->bact->dir;

            break;

        case ACTION_DEAD:

            /*** Auch kanonen fallen ***/
            
            //if( !(ygd->bact->ExtraState & EXTRA_LANDED ) ) {
            //
            //    crash.frame_time = msg->frame_time;
            //    crash.flags      = CRASH_CRASH;
            //    _methoda( o, YBM_CRASHBACTERIUM, &crash);
            //    }
            //else {
            //
            //    LONG YLS;
            //
            //    /* Wir sind gelandet und schalten noch den MegaTötler ein */
            //    state.extra_on  = EXTRA_MEGADETH;
            //    state.extra_off = state.main_state = 0;
            //    _methoda( o, YBM_SETSTATE, &state);
            //
            //    _get( o, YBA_YourLS, &YLS );
            //    if( YLS <= 0 ) ygd->bact->ExtraState |= EXTRA_DONTRENDER;
            //    }

            _methoda( o, YBM_DOWHILEDEATH, msg );
            break;
        }

    return;
    
}


_dispatcher( void, yg_YGM_INSTALLGUN, struct installgun_msg *gun )
{
/*
**  FUNCTION    Die Kanone wurde irgendwohin geseztz und ist nun nach Norden
**              ausgerichtet. Da sie aber an verschiedenen Seiten eines
**              Gebäudes stationiert sein kann, muß man sie ausrichten und
**              ihr die Schranken (Maximal-Winkel) zeigen. Das mach' mr hier.
**              Außerdem wird die kanone hier auf den Basiswinkel ausgerichtet.
**
**  INPUT       Basisvektor und Flag, ob die Kanone nach unten oder oben zeigt
**
**  RESULT
**
**  CHANGED     created 19-jan-96   8100 000C
*/

    struct ypagun_data *ygd;
    FLOAT  alpha, beta, anstieg, betrag;

    ygd = INST_DATA( cl, o );

    /*** Erschtmol merkten und normieren! ***/
    betrag = nc_sqrt( gun->basis.x*gun->basis.x + gun->basis.y*gun->basis.y +
                      gun->basis.z*gun->basis.z );

    if( betrag > 0.001 ) {
        gun->basis.x /= betrag;
        gun->basis.y /= betrag;
        gun->basis.z /= betrag;
        }

    ygd->basis = gun->basis;

    /*--------------------------------------------------------------------
    **    Ausrichten 
    **------------------------------------------------------------------*/

    /*** Basis ist lokal z ***/
    ygd->bact->dir.m31 = gun->basis.x;
    ygd->bact->dir.m32 = gun->basis.y;
    ygd->bact->dir.m33 = gun->basis.z;

    /* -------------------------------------------------------------------------
    ** y und z liegen in einer senkrechten Ebene und sind orthogonal zueinander.
    ** Ich kann also mit der Anstiegsformel arbeiten. Der Anstieg ist y durch
    ** sqrt( x²+z² ). Ist die y-Komponente von basis gleich 0, so ist der lokale
    ** y-Vektor 0.1.0
    ** -----------------------------------------------------------------------*/

    if( gun->basis.y != 0 ) {

        if( (gun->basis.x != 0) || (gun->basis.z != 0) ) {

            /*** Anstieg von Lokal z --> Anstieg von Lokal y ***/
            anstieg = gun->basis.y / nc_sqrt( gun->basis.x * gun->basis.x +
                                              gun->basis.z * gun->basis.z );
            anstieg = -1 / anstieg;
            
            /*** Daraus die y-Komponente von lokal y (nach unten, also normal) ***/
            ygd->bact->dir.m22 = nc_sqrt( anstieg * anstieg/(1 + anstieg * anstieg));

            /*** Nun noch die x und z-Komponenten von lokal z ***/
            if( gun->basis.x != 0 ) {

                beta  = 1 - ygd->bact->dir.m22 * ygd->bact->dir.m22;
                alpha = (gun->basis.z * gun->basis.z) / (gun->basis.x * gun->basis.x);
                
                ygd->bact->dir.m21 = nc_sqrt( beta / ( 1 + alpha ) );
                ygd->bact->dir.m23 = nc_sqrt( beta -
                                              ygd->bact->dir.m21 * ygd->bact->dir.m21);
                }
            else {

                beta  = 1 - ygd->bact->dir.m22 * ygd->bact->dir.m22;
                alpha = (gun->basis.x * gun->basis.x) / (gun->basis.z * gun->basis.z);

                ygd->bact->dir.m23 = nc_sqrt( beta / ( 1 + alpha ) );
                ygd->bact->dir.m21 = nc_sqrt( beta -
                                              ygd->bact->dir.m23 * ygd->bact->dir.m23);
                }
            
            /*** Vorzeichen korrigieren (log ex oder) ***/
            if( gun->basis.x < 0 ) ygd->bact->dir.m21 = -ygd->bact->dir.m21;
            if( gun->basis.z < 0 ) ygd->bact->dir.m23 = -ygd->bact->dir.m23;
            if( gun->basis.y > 0 ) {
                ygd->bact->dir.m21 = -ygd->bact->dir.m21;
                ygd->bact->dir.m23 = -ygd->bact->dir.m23;
                }
            }
        else {

            /*** Sollte nicht vorkommen, aber trotzdem... ***/
            ygd->bact->dir.m21 = 0.0;
            ygd->bact->dir.m22 = 0.0;
            ygd->bact->dir.m23 = 1.0;
            }
        }
    else {

        ygd->bact->dir.m21 = 0.0;
        ygd->bact->dir.m22 = 1.0;
        ygd->bact->dir.m23 = 0.0;
        }

    /*** Hängt das Ding nach unten? y = -y ***/
    if( gun->flags & YGFIG_HangDown ) {

        ygd->flags        |=  GUN_HangDown;
        ygd->bact->dir.m21 = -ygd->bact->dir.m21;
        ygd->bact->dir.m22 = -ygd->bact->dir.m22;
        ygd->bact->dir.m23 = -ygd->bact->dir.m23;
        }

    /* --------------------------------------------------------------------------
    ** Die jetzt gemerkte y-Achse ist die zukünftige Rotationsachse. Die
    ** lokale y-Achse der Kanone kann sich ändern. Mit dem Basisvektor und
    ** dem RotVektor habe ich alle Informationen über die Verankerung der kanone.
    ** ------------------------------------------------------------------------*/

    ygd->rot.x = ygd->bact->dir.m21;
    ygd->rot.y = ygd->bact->dir.m22;
    ygd->rot.z = ygd->bact->dir.m23;

    /*** Nun lokal x. Das ist Y x Z ***/
    ygd->bact->dir.m11 = ygd->bact->dir.m22 * ygd->bact->dir.m33 -
                         ygd->bact->dir.m23 * ygd->bact->dir.m32;
    ygd->bact->dir.m12 = ygd->bact->dir.m23 * ygd->bact->dir.m31 -
                         ygd->bact->dir.m21 * ygd->bact->dir.m33;
    ygd->bact->dir.m13 = ygd->bact->dir.m21 * ygd->bact->dir.m32 -
                         ygd->bact->dir.m22 * ygd->bact->dir.m31;

    /*** War das nicht wiedermal wunderbar einfach ???? ***/
}


/*------------------------------------------------------------------------
**      geometrics
**----------------------------------------------------------------------*/

void yg_m_mul_v(struct flt_m3x3 *m, struct flt_triple *v1, struct flt_triple *v)
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


void yg_vec_round_vec( struct flt_triple *rot, struct flt_triple *vec, FLOAT angle )
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

    yg_m_mul_v( &rm, vec, &vc );

    *vec = vc;
}

void yg_RotLokalY( struct ypagun_data *ygd, struct flt_triple *y_vec, FLOAT time )
{
    /*
    ** Wir haben nun den Vektor, der früher mal die lokale y-Achse war.
    ** Wir drehen nun die Matrix auf den vektor.
    */
    FLOAT  dist, rot_x, rot_y, rot_z, sa, ca, angle;
    struct flt_m3x3 rm, nm;

    rot_x = y_vec->y * ygd->bact->dir.m23 - y_vec->z * ygd->bact->dir.m22;
    rot_y = y_vec->z * ygd->bact->dir.m21 - y_vec->x * ygd->bact->dir.m23;
    rot_z = y_vec->x * ygd->bact->dir.m22 - y_vec->y * ygd->bact->dir.m21;

    dist = nc_sqrt( rot_x * rot_x + rot_y * rot_y + rot_z * rot_z );
    if( dist > 0.01 ) {

        rot_x /= dist;
        rot_y /= dist;
        rot_z /= dist;
        
        angle = nc_acos( ygd->bact->dir.m21 * y_vec->x +
                         ygd->bact->dir.m22 * y_vec->y +
                         ygd->bact->dir.m23 * y_vec->z );
        if( angle > ( ygd->bact->max_rot * time ) )
                      angle = ygd->bact->max_rot * time;

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
        
        nc_m_mul_m( &(ygd->bact->dir), &rm, &nm );

        ygd->bact->dir = nm;
        }
}


void yg_rot_round_lokal_x( struct Bacterium *bact, FLOAT angle)
{

    struct flt_m3x3 neu_dir, rm;
    FLOAT sin_y = sin( angle );
    FLOAT cos_y = cos( angle );


    rm.m11 = 1.0;       rm.m12 = 0.0;       rm.m13 = 0.0;
    rm.m21 = 0.0;       rm.m22 = cos_y;     rm.m23 = sin_y;
    rm.m31 = 0.0;       rm.m32 = -sin_y;    rm.m33 = cos_y;
    
    nc_m_mul_m(&rm, &(bact->dir), &neu_dir);

    bact->dir = neu_dir;
}


void yg_rot_round_lokal_y( struct Bacterium *bact, FLOAT angle)
{

    struct flt_m3x3 neu_dir, rm;
    FLOAT sin_y = sin( angle );
    FLOAT cos_y = cos( angle );


    rm.m11 = cos_y;        rm.m12 = 0.0;       rm.m13 = sin_y;
    rm.m21 = 0.0;          rm.m22 = 1.0;       rm.m23 = 0.0;
    rm.m31 = -sin_y;       rm.m32 = 0.0;       rm.m33 = cos_y;
    
    nc_m_mul_m( &rm, &(bact->dir), &neu_dir);

    bact->dir = neu_dir;
}


void yg_rot_round_rot( struct ypagun_data *ygd, FLOAT angle )
{
    FLOAT sa, ca, rot_x, rot_y, rot_z;
    struct flt_m3x3 rm, nm;

    rot_x = ygd->rot.x;
    rot_y = ygd->rot.y;
    rot_z = ygd->rot.z;

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

    nc_m_mul_m( &(ygd->bact->dir), &rm, &nm );

    ygd->bact->dir = nm;
}

void yg_dir_round_vec( struct flt_triple *rot, struct flt_m3x3 *dir, FLOAT angle )
{

    /*** Rotiert eine Matrix um einen übergebenen Vektor ***/
    FLOAT sa, ca, rot_x, rot_y, rot_z;
    struct flt_m3x3 rm, nm;

    rot_x = rot->x;
    rot_y = rot->y;
    rot_z = rot->z;

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

    nc_m_mul_m( dir, &rm, &nm );

    *dir = nm;
}


BOOL yg_CheckGround( struct ypagun_data *ygd )
{
/* -----------------------------------------------------------------
** testet, ob der Untergrund noch da ist. Wenn nicht, dann geben wir
** ein FALSE zurück.
** ---------------------------------------------------------------*/

    struct intersect_msg inter;

    inter.pnt.x = ygd->bact->pos.x;
    inter.pnt.y = ygd->bact->pos.y;
    inter.pnt.z = ygd->bact->pos.z;
    inter.flags = 0;

    /*** Global, nicht lokal y !!! ***/
    inter.vec.x = inter.vec.z = 0.0;
    if( ygd->flags & GUN_HangDown )
        inter.vec.y = -ygd->bact->pref_height;
    else
        inter.vec.y =  ygd->bact->pref_height;

    _methoda( ygd->world, YWM_INTERSECT, &inter );

    return( (BOOL) inter.insect );
}
