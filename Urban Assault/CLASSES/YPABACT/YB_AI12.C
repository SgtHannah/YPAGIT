/*
**  $Source: $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:24:47 $
**  $Locker: $
**  $Author: $
**
**  Die Intelligenzen...
**  Erster Teil mit den allgemeingültigen AI1 und AI2
**
**  (C) Copyright 1995 by Andreas Flemming
*/
#include <exec/types.h>
#include <stdio.h>

#include <math.h>

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "engine/engine.h"
#include "transform/tform.h"
#include "ypa/ypaworldclass.h"
#include "ypa/ypabactclass.h"
#include "ypa/yparoboclass.h"
#include "ypa/ypagunclass.h"
#include "input/input.h"


/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine

extern UBYTE **BactLocaleHandle;


/*-----------------------------------------------------------------*/
struct Bacterium *yb_GetEnemy( struct Cell *sector, struct Bacterium *Me,
                               FLOAT *distance, UBYTE *points );
BOOL yb_SlaveWithoutTarget( struct Bacterium *b );


/*** Globales Array ***/
WORD rel[9][2] = { {-1,-1},{0,-1},{1,-1},
                   {-1,0}, {0,0}, {1,0},
                   {-1,1}, {0,1}, {1,1} };




/*-----------------------------------------------------------------*/
_dispatcher(void, yb_YBM_AI_LEVEL1, struct trigger_logic_msg *msg)
{
/*
**  FUNCTION
**
**      Primärzielbearbeitung, Abarbeitung des Hauptauftrages,
**      abhängig vom Zustand, Anweisung der Untergebenen
**
**  INPUT
**
**      triggermsg
**
**  RESULTS
**
**      Ausgefüllte tar_vec in ybd->bact
**
**  CHANGED
**
**      8.7.95      8100000C created
*/

    struct ypabact_data *ybd;
    struct MinList *list;
    struct MinNode *nd;
    struct Bacterium *slave;
    int    count;
    struct flt_triple *vc;
    BOOL   FIGHT, UCOM;
    FLOAT  dist;
    struct settarget_msg target;
    struct getformationpos_msg gfp;
    Object *userrobo;
    struct bact_message log;

    ybd = INST_DATA( cl, o );

    /*** UCOM brauchen wir für Meldungen ***/
    _get( ybd->world, YWA_UserRobo, &userrobo );
    if( (ybd->bact.master == ybd->bact.robo) && (ybd->bact.robo == userrobo) )
        UCOM = TRUE;
    else
        UCOM = FALSE;

    /*** Sonderhack: Masse == 1, dann nur runterzählen ***/
    if( ybd->bact.mass == 1.0 ) {

        if( ybd->bact.ExtraState & EXTRA_MEGADETH ) {

            /*** runterzählen ***/
            ybd->YourLastSeconds -= msg->frame_time;
            if( ybd->YourLastSeconds < 0 )
                _methoda( ybd->world, YWM_RELEASEVEHICLE, o );
            }
        else {

            /*** Das Volk will MegaDeth und bekommt es... ***/
            struct setstate_msg state;

            state.extra_off = state.main_state = 0;
            state.extra_on  = EXTRA_MEGADETH;
            _methoda( o, YBM_SETSTATE, &state );

            ybd->YourLastSeconds = 6000;
            }

        return;
        }


    /*** Der allgemeine Energieabzug ***/
    if( ybd->bact.BactClassID != BCLID_YPAMISSY )
        _methoda( o, YBM_GENERALENERGY, msg );

    /*** Bei toten grundsätzlich YLS runterzählen ***/
    if( (ACTION_DEAD == ybd->bact.MainState) &&
        (ybd->bact.ExtraState & EXTRA_LANDED) )
        ybd->YourLastSeconds -= msg->frame_time;

    /* ------------------------------------------------------------------------
    ** Auffrischaktionen für Werte , die permanent überschrieben werden.
    ** Obwohl in Move auf Grund der Kollisionschleife die Soundwerte sowieso 
    ** geupdatet werden müssen, machen wir das hier auch, für den Fall, daß das
    ** Normalgeräusch durch andere Sachen als durch Bewegung verändert wird
    ** ----------------------------------------------------------------------*/
    ybd->bact.air_const                        = ybd->bact.bu_air_const;
    ybd->bact.sc.src[ VP_NOISE_NORMAL ].pitch  = ybd->bact.bu_pitch;
    ybd->bact.sc.src[ VP_NOISE_NORMAL ].volume = ybd->bact.bu_volume;


    /*  ------------------------------------------------------------------
    **  Wenn wir tot sind, gerade erzeugt werden, unser Ziel sich nach dem
    **  dof richtet, die Zeit um ist oder wir eine Kanone sind (die keine
    **  Hauptziele kennt), geben wir gleich an AI2 weiter
    **  ----------------------------------------------------------------*/

    if( ((ybd->bact.internal_time - ybd->bact.time_ai1) < 250 ) ||
        (TARTYPE_SIMPLE        == ybd->bact.PrimTargetType) ||
        (ybd->bact.MainState   == ACTION_DEAD)   ||
        (ybd->bact.BactClassID == BCLID_YPAGUN ) ||
        (ybd->bact.MainState   == ACTION_BEAM)   ||
        (ybd->bact.MainState   == ACTION_CREATE) ) {

        _methoda( o, YBM_AI_LEVEL2, msg );
        return;
        }

    /*** ==0 ist für andere das Signal! ***/
    ybd->bact.time_ai1    = ybd->bact.internal_time;

    /*** für den fall, daß wir nix finden... ***/
    ybd->bact.tar_vec.x = 0.0;
    ybd->bact.tar_vec.y = 0.0;
    ybd->bact.tar_vec.z = 0.0;
            
    /*** Zuerst der Festhäng-Test für alle Zustände ***/
    if( (ybd->bact.internal_time - ybd->bact.bf_time) > BF_TIME ) {

        ybd->bact.bf_time = ybd->bact.internal_time;
        _methoda( o, YBM_BREAKFREE, msg );
        }

    vc = &(ybd->bact.tar_vec);

    switch( ybd->bact.MainState ) {

        case ACTION_NORMAL:

            /* ------------------------------------------------------------
            ** Wir bewegen uns oder warten einfach nur, wenn wir kein
            ** Ziel haben. Dies ist der häufigte Zustand. Hierbei
            ** holen wir uns das Primärziel, so wir eins haben, biegen
            ** tar_vec darauf.
            ** Formationen realisieren wir, indem wir unsere Position
            ** als Hauptziel weitergeben. Da wir dann aber Schwierigkeiten
            ** bekommen können, wenn wir Zieltests unternehmen, gibt es
            ** dazu das Ziel TARTYPE_FORMATION. Die einzelnen Formations
            ** punkte berechnen wir hier, da jede Klasse andere Formationen
            ** erzwingt. 
            ** ----------------------------------------------------------*/

            if( ybd->bact.PrimTargetType != (UBYTE) TARTYPE_NONE ) {

                if( ybd->bact.PrimTargetType == (UBYTE) TARTYPE_BACTERIUM ) {

                    vc->x = ybd->bact.PrimaryTarget.Bact->pos.x-ybd->bact.pos.x;
                    vc->y = ybd->bact.PrimaryTarget.Bact->pos.y-ybd->bact.pos.y;
                    vc->z = ybd->bact.PrimaryTarget.Bact->pos.z-ybd->bact.pos.z;

                    /* ------------------------------------------------------
                    ** Position merken, um bei Verlust/Tod des bactZieles die
                    ** derzeitige Position als hauptziel nehmen zu koennen
                    ** ----------------------------------------------------*/
                    if( ACTION_DEAD != ybd->bact.PrimaryTarget.Bact->MainState )
                        ybd->bact.PrimPos = ybd->bact.PrimaryTarget.Bact->pos;
                    }
                else {
                    
                    /*** Sektor oder Formation ***/
                    vc->x = ybd->bact.PrimPos.x - ybd->bact.pos.x;
                    vc->z = ybd->bact.PrimPos.z - ybd->bact.pos.z;
                    vc->y = ybd->bact.PrimPos.y - ybd->bact.pos.y;
                    }

                /*** in Zielferne keine y-Komponente beachten ***/
                if( (dist = nc_sqrt(vc->x*vc->x+vc->y*vc->y+vc->z*vc->z) ) >
                    THINK_YPS ) vc->y = 0.0;

                /* ----------------------------------------------------------
                ** NEU: Jeder kennt sein Ziel. Der Commander hat seinen Slaves
                ** das Ziel bei SETTARGET mitgeteilt und dann kämpft jeder für
                ** sich, mit einer Ausnahme: Im Chef sitzt der Spieler.
                ** dann gibt er permanent seine Position als Ziel weiter
                ** (also einfachsterweise FORMATION_POS wie bisher). Weil
                ** Ziele aber nicht mehr automatisch weitergeleitet werden,
                ** müssen wir ein SETTARGET bei _set( o,YBA_Viewer, FALSE)
                ** machen.
                ** --------------------------------------------------------*/

                if( (ybd->bact.master == ybd->bact.robo) &&
                    (ybd->flags & YBF_Viewer) ) {

                    /*** Also los ***/                
                    if( nc_sqrt( ybd->bact.tar_vec.x * ybd->bact.tar_vec.x +
                                 ybd->bact.tar_vec.y * ybd->bact.tar_vec.y +
                                 ybd->bact.tar_vec.z * ybd->bact.tar_vec.z ) <
                                 (2 * FOLLOW_DISTANCE) )
                                  FIGHT = TRUE;
                              else
                                  FIGHT = FALSE;

                    /*** Liste holen und dann den Leuten was erzählen ***/
                    list = &(ybd->bact.slave_list);
                    for( count=0, nd=list->mlh_Head; nd->mln_Succ;
                         count++, nd=nd->mln_Succ) {

                        slave = ((struct OBNode *)nd)->bact;

                        if( slave->MainState != ACTION_DEAD ) {

                            if( FIGHT ) {

                                /*** Zielweitergabe ***/
                                target.target.bact = ybd->bact.PrimaryTarget.Bact; //egal
                                target.pos = ybd->bact.PrimPos;
                                target.priority = 0;
                                target.target_type = ybd->bact.PrimTargetType;
                                _methoda( slave->BactObject, YBM_SETTARGET, &target);
                                }
                            else {

                                /*** Alle mir nach ***/
                                gfp.count = count;
                                _methoda( o, YBM_GETFORMATIONPOS, &gfp );
                                target.pos = gfp.pos;
                                target.priority = 0;
                                target.target_type = TARTYPE_FORMATION;
                                _methoda( slave->BactObject, YBM_SETTARGET, &target);
                                }
                            }
                        }
                    }    
                }
            else {

                /* -------------------------------------------------------------
                ** Wir haben kein Ziel. Wenn aber die PrimCommandID noch gesetzt
                ** ist, dann heißt das, daß wir ein Geschwader bekämpfen und
                ** evtl. noch Leute übrig sind. Fragen wir also den Robo,
                ** ob es noch Leute dieses Geschwaders gibt und fragen wir
                ** nach dem anführer.
                ** -----------------------------------------------------------*/
                if( (ybd->bact.robo != NULL) && (ybd->bact.PrimCommandID != 0) ) {

                    target.priority = ybd->bact.PrimCommandID; // Mißbrauch
                    if( _methoda( ybd->bact.robo, YRM_GETENEMY, &target ) ) {

                        /*** Es gibt jemanden ***/
                        target.priority = 0;
                        _methoda( o, YBM_SETTARGET, &target );
                        }
                    else {

                        /*** Es gibt niemaanden mehr ***/
                        ybd->bact.PrimCommandID = 0;

                        /* -------------------------------------------
                        ** noch Sektorziel setzen, wo frueher mal das
                        ** Hauptziel war. PrimPos haelt jetzt auch die
                        ** VehiclePosition (in fight.c und target.c )
                        ** -----------------------------------------*/
                        target.priority    = 0;
                        target.target_type = TARTYPE_SECTOR;
                        target.pos         = ybd->bact.PrimPos;
                        _methoda( o, YBM_SETTARGET, &target );
                        }
                    }
                }
            break;

        case ACTION_WAIT:

            /* ------------------------------------------------------------
            ** Wir holen alle Untergebenen zu uns. Auch als Handgesteuerter
            ** kommen wir hier vorbei, wenn dof.v == 0.
            ** NEU: Weil jeder sein Ziel hat, nicht mehr machen
            ** ----------------------------------------------------------*/
            //_methoda( o, YBM_ASSEMBLESLAVES, NULL );
            break;
        }


    /*** Zusatzarbeit ***/
    if( ybd->flags & YBF_UserInput ) {

        /* --------------------------------------------------------------
        ** Wenn ich User bin, dann schalte ich das HZ ab, wenn es nicht
        ** sichtbar ist (denn bei mir wird FIGHTBACT ja nicht aufgerufen)
        ** ------------------------------------------------------------*/
        if( (TARTYPE_BACTERIUM == ybd->bact.PrimTargetType) &&
            (NULL != ybd->bact.PrimaryTarget.Bact) ) {

            if( !(ybd->bact.PrimaryTarget.Bact->Sector->FootPrint &
                 (UBYTE) (1 << ybd->bact.Owner) ) ) {

                /*** Ziel abmelden ***/
                target.priority = 0;
                target.target_type = TARTYPE_NONE;
                _methoda( o, YBM_SETTARGET, &target );
                }
            }
        }
    else {

        /* ------------------------------------------------------------------
        ** Es gibt im Usermode einen Zustand, wo ich wartend aussehe, logisch
        ** aber normal bin. Wenn danach umgeschalten wird, kann es sein,
        ** daß ich noch wartend aussehe, obwohl ich fahre.
        ** ----------------------------------------------------------------*/
        if( (VP_JUSTWAIT   == ybd->bact.vpactive) &&
            (ACTION_NORMAL == ybd->bact.MainState) ) {

            struct setstate_msg state;

            state.main_state = ACTION_NORMAL;
            state.extra_off  = state.extra_on = 0;
            _methoda( o, YBM_SETSTATE, &state );
            }
        }

    /* ------------------------------------------------
    ** Nun fragen wir mal AI2, ob es was dagegen hat...
    ** ----------------------------------------------*/

    _methoda( ybd->bact.BactObject, YBM_AI_LEVEL2, msg );

}




/*-----------------------------------------------------------------*/
_dispatcher(void, yb_YBM_AI_LEVEL2, struct trigger_logic_msg *msg)
{
/*
**  FUNCTION
**
**      Testet, ob ein Sekundärziel existiert und ermittelt den Weg
**      dorthin. Existiert noch kein Sekundärziel, so wird eins gesucht.
**
**  INPUT
**
**      triggermsg
**
**  RESULTS
**
**      Ausgefüllte tar_vec in ybd->bact
**
**  CHANGED
**
**      8.7.95      8100000C created
*/
    struct ypabact_data *ybd;
    struct MinList *list;
    struct MinNode *nd;
    FLOAT  dist = 0.0;
    struct flt_triple *vc;
    struct settarget_msg target;
    struct bact_message log;
    Object *userrobo;
    BOOL   UCOM;

    ybd = INST_DATA( cl, o );



    /* --------------------------------------------------------
    ** Aufnahme eines Nebenzieles? Wir machen das nicht, wenn:
    **      die zeit noch nicht wieder um ist,
    **      wir neutral sind, uns also nicht einmischen,
    **      wir tot oder im erzeugen sind
    **      wir eine Sache sind, der das verboten ist (SIMPLE),
    **      wir gerade flüchten  (weiter unten)
    **      wir erstmal in eine bestimmte Richtung müssen
    ** ------------------------------------------------------*/

    if( ((ybd->bact.internal_time - ybd->bact.time_ai2) < 250 ) ||
        ( ybd->bact.Owner         == 0 )     ||
        ( ybd->bact.SecTargetType == TARTYPE_SIMPLE) ||
        ( ybd->bact.MainState     == ACTION_CREATE ) ||
        ( ybd->bact.MainState     == ACTION_DEAD)    ||
        ( ybd->bact.MainState     == ACTION_BEAM) ) {

        /*** Sofort weitergeben ***/
        if (ybd->flags & YBF_UserInput)
            _methoda(o, YBM_HANDLEINPUT, msg);
        else
            _methoda(o, YBM_AI_LEVEL3, msg);

        /*** Und tschüß ***/
        return;
        }

    ybd->bact.time_ai2 = ybd->bact.internal_time;

    /* ---------------------------------------------------------------
    ** Wenn wir im Fluchtzustand sind, dann löschen (!)wir alle unsere 
    ** Nebenziele und nehmen auch keine auf
    ** Da wir nur Ziele abmelden, dürfte es mit bereits toten Slaves
    ** keine Probleme geben. 
    ** Neu: In der Naehe unseres hauptzieles haben wir das Nebenziel
    ** zurecht erhalten.
    ** -------------------------------------------------------------*/

    if( (ybd->bact.ExtraState & EXTRA_ESCAPE) &&
        (nc_sqrt( (ybd->bact.tar_vec.x * ybd->bact.tar_vec.x) +
                  (ybd->bact.tar_vec.z * ybd->bact.tar_vec.z)) >
                  FAR_FROM_SEC_TARGET ) ) {
    
        /*** Zielabmeldung nur, wenn wir nicht warten ***/
        list = &(ybd->bact.slave_list);
        target.target_type = TARTYPE_NONE;
        target.priority    = 1;
        
        for(nd=list->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ)
            _methoda( ((struct OBNode *)nd)->o, YBM_SETTARGET, &target ); 
                
        _methoda( o, YBM_SETTARGET, &target );

        /*** gleich weiter ***/
        if (ybd->flags & YBF_UserInput) 
            _methoda(o, YBM_HANDLEINPUT, msg);
        else 
            _methoda(o, YBM_AI_LEVEL3, msg);
        
        return;
        }


    /* -------------------------------------------------------------------
    ** Zwar existiert zur Zeit nur ACTION_NORMAL, aber wir machen trotzdem
    ** ein switch, falls sich das erweitert.
    ** -----------------------------------------------------------------*/

    vc = &(ybd->bact.tar_vec);

    _get( ybd->world, YWA_UserRobo, &userrobo );
    if( (ybd->bact.master == ybd->bact.robo) && (ybd->bact.robo == userrobo) )
        UCOM = TRUE;
    else
        UCOM = FALSE;

    switch( ybd->bact.MainState ) {

        /*** Teilzielsuche auch im Wartezustand! ***/

        case ACTION_WAIT:
        case ACTION_NORMAL:

            /* -------------------------------------------------------------
            ** Neu: Jeder sucht für sich die nebenziele und zwar die nächst-
            ** liegenden. Die Sache, daß der Commander das alles macht, ist
            ** zwar schneller, aber unkomfortabel und fehleranfällig.
            ** Bakteriennebenziele haben höchste priorität
            ** NEU: Wir suchen immer! Weil wir ganz einfach Sichtmeldungen
            ** brauchen, egal welche Ziele oder Aggr wir haben.
            ** -----------------------------------------------------------*/

            if( ybd->bact.internal_time - ybd->bact.time_search2 > 500 ) {

                struct getsectar_msg st;

                /*** Nu guckn mr mol ***/
                ybd->bact.time_search2 = ybd->bact.internal_time;

                /*** Nach Robo extra Suchen, wegen Wichtigkeit ***/
                st.Me         = &(ybd->bact);
                st.flags      = GST_MYPOS;
                _methoda( o, YBM_GETSECTARGET, &st );

                /* ------------------------------------------
                ** Meldung, wenn was gefunden. Ob die Meldung
                ** neu ist, entscheidet der Robo. Die Suche
                ** nach FeindRobos ist Extra (-->UserRobo)
                ** ----------------------------------------*/
                if( st.SecTarget != NULL ) {

                    if( (st.SecTarget->BactClassID != BCLID_YPAROBO) &&
                        (UCOM) &&
                        (ybd->bact.found_enemy != st.SecTarget->CommandID) &&
                        ((ybd->bact.internal_time - ybd->bact.fe_time) > FE_TIME)) {

                        ULONG rgun = 0L;

                        /*** Wenn es keine Robogun ist! ***/
                        if( BCLID_YPAGUN == st.SecTarget->BactClassID )
                            _get( st.SecTarget->BactObject, YGA_RoboGun, &rgun );

                        if( !rgun ) {

                            ybd->bact.found_enemy = st.SecTarget->CommandID;
                            ybd->bact.fe_time     = ybd->bact.internal_time;

                            log.ID    = LOGMSG_FOUNDENEMY;
                            log.para1 = st.SecTarget->CommandID;
                            log.para2 = 0;
                            log.para3 = 0;
                            log.pri   = 46;
                            log.sender = &(ybd->bact);
                            _methoda( ybd->bact.robo, YRM_LOGMSG, &log );
                            }
                        }
                    }
                
                /* -------------------------------------------------------------------
                ** Ok, da ist zum einen das normale verhalten: Nimm Nebenziele
                ** auf, wenn deine Aggression ausreicht, du nicht fluechtest
                ** und kein Nebenziel hast.
                ** Auf der anderen Seite sollte er Ziele aufnehmen, wenn er
                ** wartet. Das passiert in Aggr2 oder hoeher am bearbeiteten hauptziel
                ** (welches er dann verteidigt) und in allen Aggr am Robo, welchen
                ** er dann verteidigt.
                ** -----------------------------------------------------------------*/
                if( (ACTION_WAIT == ybd->bact.MainState) ||
                
                   ((ybd->bact.Aggression >= AGGR_SECBACT) &&
                    (!(ybd->bact.ExtraState & EXTRA_ESCAPE)) &&
                    ((ybd->bact.SecTargetType == TARTYPE_NONE) ||
                     (ybd->bact.SecTargetType == TARTYPE_SECTOR) ||
                     (ybd->bact.SecTargetType == TARTYPE_FORMATION))) ) {

                    if( st.SecTarget != NULL ) {

                        ybd->bact.SecCommandID = st.SecTarget->CommandID;

                        target.target.bact = st.SecTarget;
                        target.priority    = 1;
                        target.target_type = TARTYPE_BACTERIUM;
                        _methoda( o, YBM_SETTARGET, &target);
                        }

                    /* -------------------------------------------------------
                    ** Nun habe ich evtl. keine Nebenziele. Sollte meine Aggr.
                    ** hoch genug sein, kann ich mal testen, wem der Sektor
                    ** gehört. Achtung: Rundungsfehler am Weltrand!
                    ** Neu: Sektornebenziele sollten möglichst vom Commander
                    ** verteilt werden, um Streuungen zu vermeiden. Deshalb
                    ** gebe ich als Commander dieses Ziel an Slaves weiter.
                    ** Slaves nehmen keine Sektornebenziele auf. Die Weiter-
                    ** gabe mache ich unten.
                    ** NEU: Nur, wenn ich nicht Viewer bin! Aggr4 beißt sich
                    ** mit Userverhalten --> "Decisions"
                    ** NEU: Bei Aggr5 keine Sektornebenziele
                    ** -----------------------------------------------------*/
                    if( ((ybd->bact.internal_time - ybd->bact.time_search3) > 2000) &&
                        (ybd->bact.Aggression == AGGR_SECSEC) &&
                        (!(ybd->flags & YBF_Viewer)) &&
                        (ybd->bact.master == ybd->bact.robo) &&
                        ((TARTYPE_FORMATION == ybd->bact.SecTargetType) ||
                         (TARTYPE_NONE == ybd->bact.SecTargetType))  ) {

                        /*** Sektor auch nicht außerhalb? ***/
                        if( (ybd->bact.pos.x > 1.05*SECTOR_SIZE) &&
                            (ybd->bact.pos.x < ybd->bact.WorldX-1.05*SECTOR_SIZE) &&
                            (ybd->bact.pos.z < -1.05*SECTOR_SIZE) &&
                            (ybd->bact.pos.z > ybd->bact.WorldZ+1.05*SECTOR_SIZE)) {

                            ybd->bact.time_search3 = ybd->bact.internal_time;

                            if( ybd->bact.Sector->Owner != ybd->bact.Owner ) {

                                /* ----------------------------------------
                                ** Sektor als Ziel zuweisen, wenn noch kein
                                ** Nebenziel da ist 
                                ** --------------------------------------*/
                                target.priority    = 1;
                                target.target_type = TARTYPE_SECTOR;
                                target.pos.x       = ybd->bact.pos.x;
                                target.pos.z       = ybd->bact.pos.z;

                                //if( ybd->bact.SecTargetType == (UBYTE)TARTYPE_NONE )
                                _methoda( o, YBM_SETTARGET, &target );
                                }
                            }
                        }

                    /* ---------------------------------------------------
                    ** Bin ich Commander und habe ich ein Sektornebenziel,
                    ** so gebe ich es allen Slaves weiter, die sonst kein
                    ** NebenZiel haben. Denn nur ich durfte eins aufnehmen
                    ** -------------------------------------------------*/
                    if( (ybd->bact.master == ybd->bact.robo) &&
                        (TARTYPE_SECTOR   == ybd->bact.SecTargetType) ) {

                        struct OBNode *slave;
                        slave = (struct OBNode *) ybd->bact.slave_list.mlh_Head;
                        while( slave->nd.mln_Succ ) {

                            /*** Ziel erwünscht? ***/
                            if( (TARTYPE_NONE      == slave->bact->SecTargetType) ||
                                (TARTYPE_FORMATION == slave->bact->SecTargetType)){

                                /*** Übergeben ***/
                                target.target_type = TARTYPE_SECTOR;
                                target.pos         = ybd->bact.SecPos;
                                target.priority    = 1;
                                _methoda( slave->o, YBM_SETTARGET, &target );
                                }

                            slave = (struct OBNode *) slave->nd.mln_Succ;
                            }
                        }
                    }
                }


            /* --------------------------------------------------------------
            ** Wenn ich nun ein Ziel habe, dann biege ich tar_vec darauf hin.
            ** Nebenziele könn auch Sektoren sein!
            ** ------------------------------------------------------------*/
            if( ybd->bact.SecTargetType == (UBYTE)TARTYPE_BACTERIUM ) {

                vc->x = ybd->bact.SecondaryTarget.Bact->pos.x-ybd->bact.pos.x;
                vc->y = ybd->bact.SecondaryTarget.Bact->pos.y-ybd->bact.pos.y;
                vc->z = ybd->bact.SecondaryTarget.Bact->pos.z-ybd->bact.pos.z;
                }
            else
                if( ybd->bact.SecTargetType == (UBYTE)TARTYPE_SECTOR ) {

                    vc->x = ybd->bact.SecPos.x-ybd->bact.pos.x;
                    vc->y = ybd->bact.SecPos.y-ybd->bact.pos.y;
                    vc->z = ybd->bact.SecPos.z-ybd->bact.pos.z;
                    }

            /*** in Zielferne keine y-Komponente beachten ***/
            if( (dist = nc_sqrt(vc->x*vc->x+vc->y*vc->y+vc->z*vc->z) ) >
                THINK_YPS ) vc->y = 0.0;
    
            break;
        }

    /* --------------------------------------------------------------
    ** Wenn ich User bin, dann schalte ich das NZ ab, wenn es nicht
    ** sichtbar ist (denn bei mir wird FIGHTBACT ja nicht aufgerufen.
    ** ------------------------------------------------------------*/
    if (ybd->flags & YBF_UserInput) {

        if( (TARTYPE_BACTERIUM == ybd->bact.SecTargetType) &&
            (NULL != ybd->bact.SecondaryTarget.Bact) ) {

            if( !(ybd->bact.SecondaryTarget.Bact->Sector->FootPrint &
                 (UBYTE) (1 << ybd->bact.Owner) ) ) {

                /*** Ziel abmelden ***/
                target.priority = 1;
                target.target_type = TARTYPE_NONE;
                _methoda( o, YBM_SETTARGET, &target );
                }
            }
        }

    
    /*** Weitergeben an AI3 ***/
    if (ybd->flags & YBF_UserInput)
        _methoda(o, YBM_HANDLEINPUT, msg);
    else
        _methoda(o, YBM_AI_LEVEL3, msg);

}



_dispatcher( void, yb_YBM_GETFORMATIONPOS, struct getformationpos_msg *gfp )
{
/*
**  FUNCTION    Ermittelt die Position bezüglich des anfragenden Objektes
**
**  INPUT       count im Geschwader
**
**  RESULT      Pos in absoluten Koordinaten und in relativen Koordinaten
**
**  CHANGED     31-Oct-95       8100 000C
*/

    struct ypabact_data *ybd;
    FLOAT dof_x, dof_z;
    ULONG glied, rotte;
    struct Bacterium *chef;

    ybd  = INST_DATA(cl, o );
    chef = &(ybd->bact);

    #define ABSTAND_ROTTEN     150.0
    #define ABSTAND_GLIED      100.0

    /* ------------------------------------------------------------------------
    ** Achtung! Die Formation bezieht sich auf die lokale z-Achse. Das ist zwar
    ** nicht immer richtig, die Geschwindigkeit kann aber 0 sein!!!
    ** ----------------------------------------------------------------------*/
    
    dof_z = chef->dir.m33/nc_sqrt(chef->dir.m31*chef->dir.m31+chef->dir.m33*chef->dir.m33);
    dof_x = chef->dir.m31/nc_sqrt(chef->dir.m31*chef->dir.m31+chef->dir.m33*chef->dir.m33);

    gfp->pos.y = chef->pos.y;
    gfp->rel.y = 0.0;

    rotte = gfp->count / 3 + 1;    // Damit wir nicht neben dem GF stehen
    glied = gfp->count % 3;

    /*** der Mittelpunkt ***/
    gfp->pos.x = chef->pos.x - dof_x * (FLOAT)rotte * ABSTAND_ROTTEN;
    gfp->pos.z = chef->pos.z - dof_z * (FLOAT)rotte * ABSTAND_ROTTEN;

    /*** nun evtl. zur Seite verzweigen ***/
    if( glied == 0) {

        gfp->pos.x += dof_z * ABSTAND_GLIED;
        gfp->pos.z -= dof_x * ABSTAND_GLIED;
        }

    if( glied == 2) {

        gfp->pos.x -= dof_z * ABSTAND_GLIED;
        gfp->pos.z += dof_x * ABSTAND_GLIED;
        }

    gfp->rel.x = gfp->pos.x - ybd->bact.pos.x;
    gfp->rel.z = gfp->pos.z - ybd->bact.pos.z;
}





_dispatcher( void, yb_YBM_GETSECTARGET, struct getsectar_msg *st)
{
/* 
** FUNCTION     Das ist vollkommen neu, weil jetzt jeder seine Nebenziele
**              selbst sucht. Wir grasen alle umliegenden 9 Sektoren ab und
**              geben das nächstliegende Ziel zurück.
*/

    struct getsectorinfo_msg gsi;
    struct Bacterium *kandidat;
    struct ypabact_data *ybd;
    UBYTE  points;

    ybd = INST_DATA(cl, o);

    if( st->flags & GST_MYPOS ) {

        gsi.abspos_x = st->Me->pos.x;
        gsi.abspos_z = st->Me->pos.z;
        }
    else {

        gsi.abspos_x = st->pos_x;
        gsi.abspos_z = st->pos_z;
        }

    if( _methoda( ybd->world, YWM_GETSECTORINFO, &gsi ) ) {

        FLOAT  distance;
        int    i, j;

        distance      = 1.0 * SECTOR_SIZE;
        points        = 0;
        st->SecTarget = NULL;

        /*** Nun alle Sektoren testen ***/
        for( i = -1; i < 2; i++ ) {

            for( j = -1; j < 2; j++ ) {

                /*** Ein gültiger Sektor. Na, mal gucken ***/
                if( kandidat = yb_GetEnemy( &(gsi.sector[ i + j * ybd->bact.WSecX]),
                                            st->Me, &distance, &points ) ) {
                    st->SecTarget = kandidat;
                    }
                }
            }
        } 
}



struct Bacterium *yb_GetEnemy( struct Cell *sector,
                               struct Bacterium *Me,
                               FLOAT  *distance,
                               UBYTE  *points )
{
    /* --------------------------------------------------------------
    ** Einfach die BactListe des übergebenen Sektors scannen und dann
    **.gucken, wer böse und näher als distance ist
    ** ------------------------------------------------------------*/
    
    struct Bacterium *found_kandidat, *kandidat;
    FLOAT  d1;
    struct VehicleProto *VP_Array, *vehicle;
    UBYTE  pt;
    Object *world;
    
    _get( Me->BactObject, YBA_World, &world );
    _get( world, YWA_VehicleArray, &VP_Array );
    vehicle        = &( VP_Array[ Me->TypeID ] );
    kandidat       = (struct Bacterium *) sector->BactList.mlh_Head;
    found_kandidat = NULL;

    while( kandidat->SectorNode.mln_Succ != 0 ) {

        ULONG VIEWER;

        /* --------------------
        ** Weiter, wenn:
        **      - eine Rakete
        **      - eine Leiche
        **      - einer von mir
        **      - ein neutraler
        ** ------------------*/
        if( (kandidat->BactClassID == BCLID_YPAMISSY) ||
            (kandidat->MainState   == ACTION_DEAD) ||
            (kandidat->Owner       == Me->Owner) ||
            (kandidat->Owner       == 0) ) {

            kandidat = (struct Bacterium *) kandidat->SectorNode.mln_Succ;
            continue;
            } 
            
        /* -----------------------------------------------------
        ** am wichtigsten ist die Art des Zieles. Die Punktzahl
        ** entspricht der JobPrioritaet. handelt es sich um eine
        ** nicht unterstuetzte Art, gibts pauschal ne 5.
        ** ---------------------------------------------------*/ 
        switch( kandidat->BactClassID ) {
            case BCLID_YPABACT:     pt = vehicle->JobFightHelicopter;   break;
            case BCLID_YPATANK:
            case BCLID_YPACAR:      pt = vehicle->JobFightTank;         break;
            case BCLID_YPAFLYER:
            case BCLID_YPAUFO:      pt = vehicle->JobFightFlyer;        break;
            case BCLID_YPAROBO:     pt = vehicle->JobFightRobo;         break;
            default:                pt = 5;                             break;
            }
        
        /*** schon was besseres? ***/    
        if( *points > pt ) {      

            kandidat = (struct Bacterium *) kandidat->SectorNode.mln_Succ;
            continue;
            }

        /*** dann lohnt es sich die Entfernung zu testen ***/
        d1 =  nc_sqrt( (Me->pos.x - kandidat->pos.x ) *
                       (Me->pos.x - kandidat->pos.x ) +
                       (Me->pos.y - kandidat->pos.y ) *
                       (Me->pos.y - kandidat->pos.y ) +
                       (Me->pos.z - kandidat->pos.z ) *
                       (Me->pos.z - kandidat->pos.z ) );

        /* ------------------------------------------------------
        ** zu weit weg? Nur wenn Opfer nicht User! Weil sonst die
        ** Opfer sich seelenruhig abplatzen lassen
        ** ----------------------------------------------------*/
        _get( kandidat->BactObject, YBA_Viewer, &VIEWER );
        if( (*distance < d1) && (VIEWER==FALSE) ) {

            kandidat = (struct Bacterium *) kandidat->SectorNode.mln_Succ;
            continue;
            }

        /* ------------------------------------------------------------
        ** Wenn ich keine Kanone bin, dann teste ich, ob schon mehr als
        ** einer das Ziel hat, denn es sollten sich nicht mehr als 2
        ** darum kümmern.
        ** Dazu scanne ich die Attackerliste des Angreifers und über-
        ** prüfe die Attacker anhand ihrer Owner oder CommandID.
        ** Sehr rechenintensiv.... :-(
        **
        ** Zusatzhack: Weil sich dann leute zum teil recht prasselig
        ** hinsetzen und abballern lassen, erfolgt der test nur in
        ** hinreichend großer Entfernung zum Hauptziel.
        ** ----------------------------------------------------------*/
        if( (BCLID_YPAGUN  != Me->BactClassID) &&
            (BCLID_YPAROBO != Me->BactClassID) ) {

            FLOAT  dist;
            struct flt_triple tpos;
            BOOL   UCOM;

            // Das Hauptziel kennt meistens nur der Chef...
            if( Me->master == Me->robo ) {

                // I'm the Chief
                if( TARTYPE_SECTOR ==  Me->PrimTargetType)
                    tpos = Me->PrimPos;
                else
                    if( TARTYPE_BACTERIUM ==  Me->PrimTargetType)
                        tpos = Me->PrimaryTarget.Bact->pos;
                    else
                        tpos = Me->pos;     // weil ???
                UCOM = TRUE;
                }
            else {

                // Chef erst ermitteln
                struct Bacterium *chef;
                _get( Me->master, YBA_Bacterium, &chef );

                if( TARTYPE_SECTOR ==  chef->PrimTargetType)
                    tpos = chef->PrimPos;
                else
                    if( TARTYPE_BACTERIUM ==  chef->PrimTargetType)
                        tpos = chef->PrimaryTarget.Bact->pos;
                    else
                        tpos = Me->pos;     // weil ???
                UCOM = FALSE;
                }

            dist = nc_sqrt( (tpos.x-Me->pos.x)*(tpos.x-Me->pos.x)+
                            (tpos.z-Me->pos.z)*(tpos.z-Me->pos.z));

            // Test nur, wenn wir zuweit weg vom HZ sind
            if( dist > FAR_FROM_SEC_TARGET ) {

                struct MinList *alist;
                struct OBNode  *anode;
                ULONG  count = 0;

                _get( kandidat->BactObject, YBA_AttckList, &alist );
                anode = (struct OBNode *) alist->mlh_Head;
                while( anode->nd.mln_Succ ) {

                    struct OBNode *st_node;
                    _get( anode->o, YBA_StAttckNode, &st_node );
                    if( (anode == st_node) &&
                        (anode->bact->Owner == Me->Owner) )
                        count++;
                    if( count > 1) break; // 2 leute...
                    anode = (struct OBNode *) anode->nd.mln_Succ;
                    }

                /*** Arbeiten schon zuviele leute dran? ***/
                if( count > 1 ) {

                    kandidat = (struct Bacterium *) kandidat->SectorNode.mln_Succ;
                    continue;
                    }

                /* ------------------------------------------------------
                ** Weiterhin nehme ich in HauptZielferne keine Nebenziele
                ** auf, wenn ich Commander bin und "freie" Slaves habe
                ** ----------------------------------------------------*/
                if( UCOM && yb_SlaveWithoutTarget( Me ) ) {

                    kandidat = (struct Bacterium *) kandidat->SectorNode.mln_Succ;
                    continue;
                    }
                }
            }

        /*** Spezialtest, auf Bact-Niveau leer (siehe Gun oder tank) ***/
        if( !_methoda( Me->BactObject, YBM_TESTSECTARGET, kandidat )) {

            kandidat = (struct Bacterium *) kandidat->SectorNode.mln_Succ;
            continue;
            }


        /*** Der ist gut ***/
        found_kandidat = kandidat;
        *distance      = d1;
        *points        = pt; 
        kandidat = (struct Bacterium *) kandidat->SectorNode.mln_Succ;
        }

    return( found_kandidat );
}


BOOL yb_SlaveWithoutTarget( struct Bacterium *bact )
{
    /*** Gibt TRUE zurück, wenn mindestens ein Slave kein Nebenziel hat ***/
    struct OBNode *slave;

    slave = (struct OBNode *) bact->slave_list.mlh_Head;
    while( slave->nd.mln_Succ ) {

        /*** BactZiel hat höchste priorität, deshalb auf !BACT testen ***/
        if( slave->bact->SecTargetType != TARTYPE_BACTERIUM )
            return( TRUE );

        slave = (struct OBNode *) slave->nd.mln_Succ;
        }

    return( FALSE );
}
