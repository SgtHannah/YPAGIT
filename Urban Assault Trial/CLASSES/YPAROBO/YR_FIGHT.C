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
#include "nucleus/math.h"
#include "engine/engine.h"
#include "transform/tform.h"
#include "ypa/ypaworldclass.h"
#include "ypa/ypabactclass.h"
#include "input/input.h"
#include "ypa/yparoboclass.h"


/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine


void yr_RoboFightIntruder( struct yparobo_data *yrd, struct trigger_logic_msg *msg );
struct Bacterium *yr_GetNearestEnemy( struct yparobo_data *yrd);

/*** Sowas sollte global sein... ***/
WORD frelpos[18] = {-1,-1,  0,-1,  1,-1,  -1,0,  0,0,  1,0,  -1,1,  0,1,  1,1};

//struct Bacterium *yr_GetNearestEnemy( struct yparobo_data *yrd)
//{
///*
//**  FUNCTION    Nebenzielaufnahme. Sollte nur (!) für feindliche Objekte
//**              im ViewBereich (oder noch weniger) genommen werden.
//**              Fungiert hier im Gegensatz zu allen anderen Bakterien
//**              parallel zum Hauptziel. Ist das Opfer aus dem Bereich
//**              raus, wird es abgemeldet.
//**              Überlagert keinen tar-Vec oder so ähnliches zeugs..
//*/
//    struct Cell *my_sector, *sector;
//    struct Bacterium *enemy, *merke_enemy;
//    FLOAT  d, merke_d;
//    int    i;
//
//
//    /*** Initialisierungen ***/
//    my_sector   = yrd->bact->Sector;
//    merke_enemy = NULL;
//    merke_d     = 0;
//
//
//    for( i=0; i<9; i++) {
//
//        /*** Nu emol 'n Sektor agucke ***/
//        sector = &(my_sector[ frelpos[ 2*i+1 ] * yrd->bact->WSecX + frelpos[ 2*i ] ]);
//        enemy  = (struct Bacterium *) sector->BactList.mlh_Head;
//
//        while( enemy->SectorNode.mln_Succ ) {
//
//            /*** es sind noch Leute da. Sind das Feinde? ***/
//            if( (enemy->Owner       == yrd->bact->Owner) ||
//                (enemy->MainState   == ACTION_DEAD) ||
//                (enemy->BactClassID == BCLID_YPAMISSY) ||
//                (enemy->Owner       == 0) ) {
//
//                /*** Interessiert uns nicht. Weiter ***/
//                enemy = (struct Bacterium *) enemy->SectorNode.mln_Succ;
//                continue;
//                }
//
//            /*** Feind da, ist der auch näher? ***/
//            d = nc_sqrt((enemy->pos.x-yrd->bact->pos.x)*(enemy->pos.x-yrd->bact->pos.x)+
//                        (enemy->pos.y-yrd->bact->pos.y)*(enemy->pos.y-yrd->bact->pos.y)+
//                        (enemy->pos.z-yrd->bact->pos.z)*(enemy->pos.z-yrd->bact->pos.z));
//
//            if( (d < merke_d) || (merke_enemy == NULL) ) {
//
//                merke_d     = d;
//                merke_enemy = enemy;
//                }
//
//            /*** next please! ***/
//            enemy = (struct Bacterium *) enemy->SectorNode.mln_Succ;
//            }
//        }
//
//    return( merke_enemy );
//}
//
//
//void yr_RoboFightIntruder( struct yparobo_data *yrd, struct trigger_logic_msg *msg )
//{
///*
//** FUCTION  fragt mittels GetNearestEnemy einen Eindringling ab und
//**          feuert eine rakete drauf ab. Wir nehmen ihn nicht als Ziel,
//**          wenn er überlebt, finden wir ihn wieder. Wir machen vorerst keine
//**          Waffenauswahl, sondern nehmen auto_ID.
//*/
//
//    struct firemissile_msg fire;
//    struct Bacterium *enemy;
//    UBYTE  merke_gun;
//    FLOAT  angle, merke_angle, distance;
//    struct flt_triple schuss;
//    int    i;
//
//    if( enemy = yr_GetNearestEnemy( yrd ) ) {
//
//        /*
//        ** Feind da. Jetzt sind wir der Robo und haben yppsn Waffen, Woraus
//        ** wir eine auswählen müssen. Alle Waffen haben eine Position. Wir nehmen
//        ** die, deren  Abschußrichtung mit der Richtung der Kanone möglichst nahe
//        ** kommen soll. Suchen wir zuerst die günstigste Kanone.
//        */
//
//        schuss.x = enemy->pos.x - yrd->bact->pos.x;
//        schuss.y = enemy->pos.y - yrd->bact->pos.y;
//        schuss.z = enemy->pos.z - yrd->bact->pos.z;
//
//        distance = nc_sqrt( schuss.x*schuss.x + schuss.y*schuss.y + schuss.z*schuss.z);
//        if( distance < 0.0001 ) return;
//
//        schuss.x /= distance;
//        schuss.y /= distance;
//        schuss.z /= distance;
//
//        merke_gun   = 0xFF;
//        merke_angle = 6.28; // für'n Compiler...
//
//        for( i=0; i<8; i++ ) {
//
//            if( yrd->gun[ i ].weapon != 0xFF ) {
//
//                angle = nc_acos( schuss.x * yrd->gun[ i ].dir.x +
//                                 schuss.y * yrd->gun[ i ].dir.y +
//                                 schuss.z * yrd->gun[ i ].dir.z );
//
//                if( (i==0) || (angle < merke_angle) ) {
//
//                    merke_gun   = i;
//                    merke_angle = angle;
//                    }
//                }
//            }
//
//        /*** Kanone auf jeden Fall gefunden, wenn eine da war (muß nicht sein!) ***/
//        if( merke_gun != 0xFF ) {
//
//            /*** Na also, ist doch was da! --> FEUER! ***/
//
//            fire.dir.x         = yrd->gun[ merke_gun ].dir.x;
//            fire.dir.y         = yrd->gun[ merke_gun ].dir.y;
//            fire.dir.z         = yrd->gun[ merke_gun ].dir.z;
//            fire.target_type   = TARTYPE_BACTERIUM;
//            fire.target.bact   = enemy;
//            fire.wtype         = yrd->gun[ merke_gun ].weapon;
//            fire.global_time   = msg->global_time;
//            fire.start.x       = yrd->gun[ merke_gun ].pos.x;
//            fire.start.y       = yrd->gun[ merke_gun ].pos.y;
//            fire.start.z       = yrd->gun[ merke_gun ].pos.z;
//            fire.flags         = 0;
//            _methoda( yrd->bact->BactObject, YBM_FIREMISSILE, &fire );
//            }
//        }
//
//}
