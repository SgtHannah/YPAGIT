/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_listview.c,v $
**  $Revision: 38.8 $
**  $Date: 1996/03/21 20:30:55 $
**  $Locker:  $
**  $Author: floh $
**
**  Routinen für das Abspeichern von Spielständen
**
**  (C) Copyright 1995 by Flemming aus Schwarzenberg
*/
#include <exec/types.h>
#include <exec/memory.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include "nucleus/nucleus2.h"
#include "engine/engine.h"
#include "visualstuff/ov_engine.h"
#include "bitmap/displayclass.h"
#include "input/inputclass.h"
#include "input/idevclass.h"
#include "bitmap/bitmapclass.h"
#include "ypa/ypaworldclass.h"
#include "ypa/guilist.h"
#include "ypa/ypagui.h"
#include "ypa/ypagameshell.h"
#include "ypa/yparoboclass.h"
#include "ypa/ypagunclass.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_input_engine
_extern_use_ov_engine
_extern_use_audio_engine


/*** Brot-O-Typen ***/
#include "yw_protos.h"
#include "yw_gsprotos.h"


/*** globale Variable ***/
ULONG   actlevel;
Object  *ActualVehicle;
struct  Bacterium *ActualBacterium;  // bezieht sich auf ActualVehicle
Object  *ActualCommander;
Object  *ActualRobo;
Object  *ViewerRobo;
ULONG   superitem;

LONG    EXTRA_VIEWER, VIEWER_NUMBER;

/* ---------------------------------------------------------------------
** NEU: Die Zähler für geschwader und Vehicle müssen beim Laden angepaßt
**      werden, damit es zu keinen Überschneidungen kommt!
** -------------------------------------------------------------------*/
extern ULONG DeathCount;
       ULONG CommandCount;

/*---------------------------------------------------------------------------
**                          Extra-Loaders und -Savers
**-------------------------------------------------------------------------*/


BOOL yw_SaveLevelStatus(  FILE *F, struct ypaworld_data *ywd, ULONG num )
{
    /*** Speichert einen Levelstatus ***/
    char str[ 300 ];

    /*** Eröffnung ***/
    sprintf( str, "\nbegin_levelstatus %ld\n", num );
    _FWrite( str, strlen( str ), 1, F );

    /*** Der Status höchstselbst ***/
    sprintf( str, "    status = %ld\n", ywd->LevelNet->Levels[ num ].status );
    _FWrite( str, strlen( str ), 1, F );

    /*** Abschluß ***/
    sprintf( str, "end\n\n" );
    _FWrite( str, strlen( str ), 1, F );

    return( TRUE );
}


BOOL yw_SaveAllLevelStatus(  FILE *F, struct ypaworld_data *ywd )
{
    ULONG i;

    for( i = 0; i < MAXNUM_LEVELS; i++ ) {

        /*** ist der Level ok? ***/
        if( LNSTAT_INVALID != ywd->LevelNet->Levels[ i ].status ) {

            /*** schreiben ***/
            if( !yw_SaveLevelStatus( F, ywd, i ) ) return( FALSE );
            }
        }

    /*** war alles ok ***/
    return( TRUE );
}


BOOL yw_SaveAllBuddies(  FILE *F, struct ypaworld_data *ywd )
{
    /*** speichert alle Fahrzeuge aus den gates ***/
    ULONG i;

    for( i = 0; i < ywd->Level->NumBuddies; i++) {

        /*** neischreiben ***/
        if( !yw_SaveBuddy( F, ywd, i ) ) return( FALSE );
        }

    /*** Na, das ging noch mal gut ***/
    return( TRUE );
}


BOOL yw_SaveBuddy(  FILE *F, struct ypaworld_data *ywd, ULONG num  )
{
    /*** Speichert so einen Buddie ab. Naja. Hm. ja. Ähem. ***/
    char str[ 300 ];

    /*** Eröffnung. Nummer interessiert nicht ***/
    sprintf( str, "\nbegin_buddy\n" );
    _FWrite( str, strlen( str ), 1, F );

    /*** Die CommandID ***/
    sprintf( str, "    commandid = %ld\n", ywd->Level->Buddies[ num ].CommandID );
    _FWrite( str, strlen( str ), 1, F );

    /*** Die TypeID ***/
    sprintf( str, "    type      = %d\n", ywd->Level->Buddies[ num ].TypeID );
    _FWrite( str, strlen( str ), 1, F );

    /*** Die Energie, die er noch hatte ***/
    sprintf( str, "    energy    = %ld\n", ywd->Level->Buddies[ num ].Energy );
    _FWrite( str, strlen( str ), 1, F );

    /*** Abschluß ***/
    sprintf( str, "end\n\n" );
    _FWrite( str, strlen( str ), 1, F );

    return( TRUE );
}


ULONG yw_ParseLevelStatus( struct ScriptParser *parser )
{
    /* -------------!!!!!!!!!!!!!!!!!!!!!!!-----------------------
    ** Parst nach Levelstatusinformationen. Falls eine "gefunden"-
    ** Meldung erwünscht ist, muß store[ 0 ] != 0 sein und wird
    ** dann als UBYTE-Pointer interpretiert. Darauf odern wir
    ** die Meldung.
    ** ---------------------------------------------------------*/
    struct ypaworld_data *ywd;

    if( parser->target ) ywd = parser->target;

    if( PARSESTAT_READY == parser->status ) {

        /*** Wir sind bereit für etwas neues ***/
        if( stricmp( parser->keyword, "begin_levelstatus") == 0 ) {

            /*** Das ist etwas für uns ***/
            parser->status = PARSESTAT_RUNNING;

            /*** Levelnummer merken ***/
            actlevel = atol( parser->data );

            return( PARSE_ENTERED_CONTEXT );
            }
        else {

            /*** Damit kann ich nichts anfangen ***/
            return( PARSE_UNKNOWN_KEYWORD );
            }
        }
    else {

        /*** Wir bearbeiten das schon. Ist es das Ende? ***/
        if( stricmp( parser->keyword, "end") == 0 ) {

            /*** Das Ende naht! ***/
            parser->status = PARSESTAT_READY;
            return( PARSE_LEFT_CONTEXT );
            }
        else {

            /*** Es sollte also ein  normales keyword sein ***/
            if( parser->target ) {

                /*** Found-Meldung, wenn erwünscht ***/
                if( parser->store[ 0 ] ) {

                    UBYTE *FoundFlag;
                    FoundFlag   = (UBYTE *)(parser->store[ 0 ]);
                    *FoundFlag |= DM_SCORE;
                    }

                if( stricmp( parser->keyword, "status" ) == 0 ) {

                    /*** Existiert der Level? ***/
                    if( ywd->LevelNet->Levels[ actlevel ].status != LNSTAT_INVALID) {

                        /*** in actlevel steht die Nr., zu der die Info gehört ***/
                        ywd->LevelNet->Levels[ actlevel ].status =
                             atol( parser->data );
                        }
                    }
                else return( PARSE_UNKNOWN_KEYWORD );
                }

            return( PARSE_ALL_OK );
            }
        }
}


ULONG yw_ParseBuddy( struct ScriptParser *parser )
{
    struct ypaworld_data *ywd;

    if( parser->target ) ywd = parser->target;

    if( PARSESTAT_READY == parser->status ) {

        /*** Wir sind bereit für etwas neues ***/
        if( stricmp( parser->keyword, "begin_buddy") == 0 ) {

            /*** Das ist etwas für uns ***/
            parser->status = PARSESTAT_RUNNING;
            return( PARSE_ENTERED_CONTEXT );
            }
        else {

            /*** Damit kann ich nichts anfangen ***/
            return( PARSE_UNKNOWN_KEYWORD );
            }
        }
    else {

        /*** Wir bearbeiten das schon. Ist es das Ende? ***/
        if( stricmp( parser->keyword, "end") == 0 ) {

            /*** Das Ende naht! ***/
            parser->status = PARSESTAT_READY;

            /*** Hier erhöhe ich NumBuddies, welches mir als Offset dient ***/
            if( parser->target ) ywd->Level->NumBuddies++;
            return( PARSE_LEFT_CONTEXT );
            }
        else {

            /*** Es sollte also ein  normales keyword sein ***/
            if( parser->target ) {

                if( stricmp( parser->keyword, "commandid") == 0 ) {

                    ywd->Level->Buddies[ ywd->Level->NumBuddies ].CommandID =
                        atol( parser->data );
                    }
                else {

                if( stricmp( parser->keyword, "type") == 0 ) {

                    ywd->Level->Buddies[ ywd->Level->NumBuddies ].TypeID =
                        atoi( parser->data );
                    }
                else {

                if( stricmp( parser->keyword, "energy") == 0 ) {

                    ywd->Level->Buddies[ ywd->Level->NumBuddies ].Energy =
                        atol( parser->data );
                    }
                else return( PARSE_UNKNOWN_KEYWORD );                
                } } }

            return( PARSE_ALL_OK );
            }
        }
}

/*  -------------------------------------------------------------------------
**
**  D I E  L E V E L S P E Z I F I S C H E N  S P E I C H E R R O U T I N E N
**
**  -----------------------------------------------------------------------*/


_dispatcher( BOOL, yw_YWM_SAVEGAME, struct saveloadgame_msg *slg )
{
/*
**  FUNCTION    Frontend für das Abspeichern eines einem level und einem
**              Spieler eindeutig zugeordneten Spielstandes.
**              Es wird ein Script mit den Geschwader abgespeichert, weiterhin
**              müssen aktuelle Building-/Owner- und EnergyMaps gesichert
**              werden.
**
**  INPUT       GSR und der Name des Files
**
**  OUTPUT      TRUE, wenn alles ok war.
**
**  CHANGED     keine Buddies mehr, weil jetzt eh alle erst am Ende "rein-
**              gezogen" werden
**
**              Name wird extern gebastelt, weil es verschiedene savegames gibt
**
**              NEU: Ich kann Files zu verschiedenen Zwecken abspeichern,
**              die dann verschiedene Namen haben. Die namen werden EXTERN
**              gebastelt und alle Infos werden daraus gezogen.
**              ywd->Level->Num ist in diesen Fällen immer ausgefüllt
*/

    struct ypaworld_data *ywd;
    FILE   *SGF;
    char   filename[ 300 ];
    BOOL   all_ok = TRUE, buildinfo = TRUE, levelstatus;
    BOOL   history = TRUE;
    BOOL   userdata = TRUE;
    int    i;

    ywd = INST_DATA( cl, o );

    /*** INFOS AUS DEM NAMEN ***/
    
    /*** normales SaveGame? Dann "Older-Flag" loeschen ***/
    if( strstr( slg->name, ".sgm" ) ||
        strstr( slg->name, ".SGM" )) {
        
        char f[ 300 ];
        sprintf( f, "save:%s/sgisold.txt", ywd->gsr->UserName );
        _FDelete( f );
        }

    /*** finale Savegames ohne BuildInfo ***/
    if( strstr( slg->name, ".fin" ) ||
        strstr( slg->name, ".FIN" )) {

        levelstatus = FALSE;
        buildinfo   = FALSE;
        userdata    = FALSE;

        /*** FIN? --> Durch ein Beamgate raus --> MAxRoboEnergy akt. ***/
        ywd->MaxRoboEnergy  = ywd->URBact->Maximum;
        ywd->MaxReloadConst = ywd->URBact->RoboReloadConst;
        }
    else {

        levelstatus = TRUE;
        buildinfo   = TRUE;
        userdata    = TRUE;
        }

    if( strstr( slg->name, ".fin" ) ||
        strstr( slg->name, ".FIN" )) {
        history = FALSE;
        }
    else {
        history = TRUE;
        }

    /*** Filename basteln und Leerzeichen rausfiltern ***/
    sprintf( filename, "%s\0", slg->name );

    /*** File erzeugen ***/
    SGF = _FOpen( filename, "w" );
    if( !SGF ) {

        _LogMsg( "Unable to open savegame file\n");
        _FClose( SGF );
        return( FALSE );
        }

    /* ------------------------------------------------------------
    ** Zuerst die allgemeinen Bauinfos, die auf alle Vehicle wirken
    ** und somit auch zuerst geparst werden
    ** ----------------------------------------------------------*/
    if( buildinfo ) {

        if( !yw_SaveBuildInfo( ywd, SGF ) ) all_ok = FALSE;
        }
        
    /*** Userdaten nicht im Final savegame ***/
    if( userdata ) {

         if( !yw_WriteUserData(SGF,"blabla",ywd->gsr )) all_ok = FALSE;
         }
         
    /* ---------------------------------------------------------
    ** Abspeichern. Die Maps zuerst! Damit beim Laden (und damit
    ** verbundenen gebäudebauen) noch keine Robos da sind!
    ** -------------------------------------------------------*/
    if( all_ok ) {

        // FIXME FLOH
        if( yw_SaveLevelNum( ywd, SGF ) )
            if( yw_SaveWorld( ywd, SGF ) )
                if( yw_SaveVehicles( ywd, SGF ) )
                    if( yw_SaveWunderInfo( ywd, SGF ) )
                        if( yw_SaveKWFactor( ywd, SGF ) )
                            if( yw_SaveGlobals( ywd, SGF ) )
                                if( yw_SaveSuperBombs( ywd, SGF ) )
                                    all_ok = TRUE;
        }

    /*** Sachen, die nicht immer abgespeichert werden ***/
    if( all_ok && levelstatus ) {

        if( !yw_SaveAllLevelStatus( SGF, ywd ) ) all_ok = FALSE;
        }

    /*** History-Buffer ***/
    if( all_ok && history ) {

        yw_SaveHistory(ywd,SGF);
        yw_SaveMasks(ywd, SGF );
        }

    /*** und tschüß ***/
    _FClose( SGF );
    return( all_ok );
}


BOOL yw_SaveSuperBombs( struct ypaworld_data *ywd, FILE *SGF )
{
    /*** Fuer jede Bombe einen Eintrag ***/
    int i;

    for( i = 0; i < ywd->Level->NumItems; i++ ) {

        fprintf( SGF, "\nbegin_superbomb\n");
        fprintf( SGF, "    num               = %d\n\0", i);
        fprintf( SGF, "    status            = %d\n\0", ywd->Level->Item[ i ].status );
        fprintf( SGF, "    active_timestamp  = %d\n\0", ywd->Level->Item[ i ].active_timestamp );
        fprintf( SGF, "    trigger_timestamp = %d\n\0", ywd->Level->Item[ i ].trigger_timestamp );
        fprintf( SGF, "    activated_by      = %d\n\0", ywd->Level->Item[ i ].activated_by );
        fprintf( SGF, "    countdown         = %d\n\0", ywd->Level->Item[ i ].countdown );
        fprintf( SGF, "    last_ten_sec      = %d\n\0", ywd->Level->Item[ i ].last_ten_sec );
        fprintf( SGF, "    last_sec          = %d\n\0", ywd->Level->Item[ i ].last_sec );
        fprintf( SGF, "    radius            = %d\n\0", ywd->Level->Item[ i ].radius );
        fprintf( SGF, "    last_radius       = %d\n\0", ywd->Level->Item[ i ].last_radius );
        fprintf( SGF, "end\n");
        }
}


BOOL yw_SaveLevelNum( struct ypaworld_data *ywd, FILE *SGF )
{
    fprintf( SGF, "\nbegin_levelnum\n");
    fprintf( SGF, "    levelnum = %d\n", ywd->Level->Num);
    fprintf( SGF, "end\n");

    return( TRUE );
}


BOOL yw_SaveMasks( struct ypaworld_data *ywd, FILE *SGF )
{
    fprintf( SGF, "\nbegin_masks\n");
    fprintf( SGF, "    ownermask = %d\n", ywd->Level->OwnerMask);
    fprintf( SGF, "    usermask  = %d\n", ywd->Level->UserMask);
    fprintf( SGF, "end\n");

    return( TRUE );
}


BOOL yw_SaveWorld( struct ypaworld_data *ywd, FILE *SGF )
{
    /*  -------------------------------------------------------
    **  Speichert Weltdaten, wie die verschiedenen Maps und Den 
    **  derzeitigen Energiehaushalt ab. Da nehmen wir Flohs
    **  Zeug zum Saven von Resourcen
    **  -----------------------------------------------------*/

    /*** Owner-Map ***/
    yw_SaveActualOwnerMap( ywd, SGF );

    /*** Building-Map ***/
    yw_SaveActualBuildingMap( ywd, SGF );
    
    /*** Energy-Map ***/
    yw_SaveActualEnergyMap( ywd, SGF );
    
    /*** Erfolg ***/
    return( TRUE );
}


BOOL yw_SaveActualOwnerMap( struct ypaworld_data *ywd, FILE *SGF )
{
/*
**  FUNCTION    speichert die aktuelle Ownermap ab. Dazu erzeuge
**              ich ein BitmapObjekt, indem ich die OwnerMap neu
**              erzeuge. Ich brauche ja eine resource, anders geht
**              das nicht. Anschließend schreibe ich die Resource,
**              nachdem ich sie verändert habe, in ein anderes File,
**              nämlich in das, welches ich will. Das ist chaotisch,
**              aber ich sehe keinen Weg, eine leere Resource zu erzeugen
**              statt zu laden...
**
**              So elegant und einfach wie bei der  Buildingmap geht das
**              nicht, weil sonst jeder Owner-Wechsel registrert würde,
**              Map bleibt aber bestehen.
**
**  INPUT       alle Daten aus ywd
**
**  CHANGED
*/
    struct VFMBitmap *BMap;
    int i;
    Object *bmpo;

    /*** temporäres Bitmap-Objekt erzeugen ***/
    bmpo = _new("bitmap.class",
                RSA_Name,   "temp_owner_map",
                BMA_Width,  ywd->MapSizeX,
                BMA_Height, ywd->MapSizeY,
                TAG_DONE);
    if (bmpo) {
        _get( bmpo, BMA_Bitmap, &BMap );
        for( i = 0; i < (ywd->MapSizeX * ywd->MapSizeY); i++ ) {
            UBYTE *data         = (UBYTE *)(BMap->Data);
            struct Cell *sector = ywd->CellArea;
            data[ i ] = sector[ i ].Owner;
            }
        /*** Abspeichern ***/
        fprintf( SGF,"\nbegin_ownermap\n");
        yw_SaveBmpAsAscii(ywd,bmpo,"        ",SGF );
        fprintf( SGF,"end\n");
        _dispose(bmpo);
    } else return(FALSE);
    return( TRUE );
}


BOOL yw_SaveActualBuildingMap( struct ypaworld_data *ywd, FILE *SGF )
{
/*
**  FUNCTION    NEU: ASCII speichern
**
**              NEU: Die daten vom Designer werden mit durchgeschleift.
**              folglich gibt es aktuelle Maps, auf die ich lediglich
**              ein Save anwenden muß.
**
**  INPUT       alle Daten aus ywd
**
**  CHANGED
*/

    /*** Abspeichern ***/
    fprintf( SGF,"\nbegin_buildingmap\n");
    yw_SaveBmpAsAscii(ywd,ywd->BuildMap,"        ",SGF );
    fprintf( SGF,"end\n");
    
    return( TRUE );
}



BOOL yw_SaveActualEnergyMap( struct ypaworld_data *ywd, FILE *SGF )
{
/*
**  FUNCTION    speichert die aktuelle Energymap ab. Dazu erzeuge
**              ich ein leeres BitmapObjekt und schreibe dort alle
**              Owner rein. Anschließend speichere ich es mittels
**              RSM_SAVE.
**
**  INPUT       alle Daten aus ywd
**
**  CHANGED
*/
    Object *BMO = NULL;
    BOOL   retvalue = TRUE;

    if( BMO = _new( "bitmap.class", BMA_Width,  3 * ywd->MapSizeX,
                                    BMA_Height, 3 * ywd->MapSizeY,
                                    RSA_Name,   "ActualEnergyMap",
                                    TAG_DONE ) ) {

        struct VFMBitmap *BMap;
        int    i;

        /*** VFMBitmap holen und ->Data ausfüllen ***/
        _get( BMO, BMA_Bitmap, &BMap );

        for( i = 0; i < (ywd->MapSizeX * ywd->MapSizeY); i++ ) {

            int    j, k;
            UBYTE *data         = (UBYTE *)(BMap->Data);
            struct Cell *sector = ywd->CellArea;

            /*** Es sind 9 Daten, die zu übertragen sind ***/
            for( j = 0; j < 3; j++ )
                for( k = 0; k < 3; k++ )
                    data[ 9 * i + 3 * j + k ] = sector[ i ].SubEnergy[ j ][ k ];
            }

        /*** Abspeichern ***/
        fprintf( SGF,"\nbegin_energymap\n");
        yw_SaveBmpAsAscii(ywd,BMO,"        ", SGF);
        fprintf( SGF,"end\n");
        retvalue = TRUE;
        }
    else {

        _LogMsg("game save error: Unable to create bmo for saving owner\n");
        retvalue = FALSE;
        }

    /*** Freigabe vom Zeuch ***/
    if( BMO ) _dispose( BMO );

    return( retvalue );
}


BOOL yw_SaveVehicles( struct ypaworld_data *ywd, FILE *SGF )
{
    /*  --------------------------------------------------------
    **  Speichert alle vehicle ab, geht dabei alle Robos, danach
    **  alle Commander und dann alle Slaves durch.
    **  ACHTUNG, um die gleiche Geschwaderstruktur zu bekommen,
    **  parsen wir von hinten nach vorn!
    **  ------------------------------------------------------*/
    struct OBNode *robo;

    for( robo = (struct OBNode *) ywd->CmdList.mlh_TailPred;
         robo->nd.mln_Pred;
         robo = (struct OBNode *) robo->nd.mln_Pred ) {

        struct OBNode *cmd;

        /* ----------------------------------------------------------
        ** Krempel und evtl. auch tote Robos rausfiltern rausfiltern,
        ** weil unter den logisch toten ja leute hängen...
        ** --------------------------------------------------------*/
        if( robo->bact->MainState == ACTION_DEAD ) continue;

        /*** Robo abspeichern ***/
        if( !yw_SaveRobo( robo, SGF )) return( FALSE );

        /*** Commander abarbeiten ***/
        for( cmd = (struct OBNode *) robo->bact->slave_list.mlh_TailPred;
             cmd->nd.mln_Pred;
             cmd = (struct OBNode *) cmd->nd.mln_Pred ) {

            struct OBNode *slave;
            LONG   robo_gun = 0;

            /*** Commander  abspeichern, keine guns ***/
            if( BCLID_YPAGUN == cmd->bact->BactClassID )
                _get( cmd->o, YGA_RoboGun, &robo_gun );

            if( !robo_gun ) {
                if( !yw_SaveCommander( cmd, SGF )) return( FALSE ); }
            else {

                /*** wenigstens Viewer testen ***/
                LONG view;
                _get( cmd->o, YBA_Viewer, &view );
                if( view )
                    if( !yw_SaveExtraViewerInfo( cmd, SGF )) return( FALSE );
                }

            /*** Die Untergebenen ***/
            for( slave = (struct OBNode *) cmd->bact->slave_list.mlh_TailPred;
                 slave->nd.mln_Pred;
                 slave = (struct OBNode *) slave->nd.mln_Pred ) {

                LONG robo_gun = 0;

                /*** Abspeichern des Slaves ***/
                if( BCLID_YPAGUN == slave->bact->BactClassID )
                    _get( slave->o, YGA_RoboGun, &robo_gun );

                if( !robo_gun ) {
                    if( !yw_SaveSlave( slave, SGF )) return( FALSE ); }
                else {

                    /*** wenigstens Viewer testen ***/
                    LONG view;
                    _get( slave->o, YBA_Viewer, &view );
                    if( view )
                        if( !yw_SaveExtraViewerInfo( slave, SGF )) return( FALSE );
                    }
                }
            }
        }

    return( TRUE );
}


BOOL yw_SaveExtraViewerInfo( struct OBNode *ex, FILE *SGF )
{
    /* ----------------------------------------------------------
    ** Es kann sein, daß jemand viewer ist, obwohl er nicht
    ** abgespeichert werden wird. Trotzdem müssen wir uns merken,
    ** daß er Viewer ist. Wir merken uns die Art und eine
    ** Nummer. Das ist universell, auch wenn es derzeit nur
    ** die Roboflaks nutzen
    ** --------------------------------------------------------*/
    char   buffer[300 ];

    if( BCLID_YPAGUN == ex->bact->BactClassID ) {

        struct yparobo_data *yrd;
        int    i, j;
        BOOL   found = FALSE;
        Object *ro;

        /*** na dann handelt es sich um eine Bordkanone ***/
        sprintf( buffer, "\nbegin_extraviewer\n");
        _FWrite( buffer, strlen( buffer ), 1, SGF );

        /*** Nummer ermitteln ***/
        ro  = ex->bact->robo;
        yrd = INST_DATA( ((struct nucleusdata *)ro)->o_Class, ro );

        for( i = 0; i < NUMBER_OF_GUNS; i++ ) {

            if( yrd->gun[ i ].go == ex->o ) {

                found = TRUE; j = i; break; }
            }

        if( !found ) {

            /*** So was ?! ***/
            _LogMsg("Error: Gun not found in yrd!\n");
            return( FALSE );
            }

        /*** Rasuschreiben ***/
        sprintf( buffer, "    kind = robogun\n");
        _FWrite( buffer, strlen( buffer ), 1, SGF );
        sprintf( buffer, "    number = %d\n", i);
        _FWrite( buffer, strlen( buffer ), 1, SGF );

        sprintf( buffer, "end\n\n");
        _FWrite( buffer, strlen( buffer ), 1, SGF );
        return( TRUE );
        }
    else {

        /*** keine Ahnung, was das soll! ***/
        return( FALSE );
        }
}

BOOL yw_SaveRobo( struct OBNode *robo, FILE *SGF )
{
    /*  ---------------------------------------------------------
    **  Sichert einen Robo. Dabei müsen wir zusätzliche Daten
    **  abspeichern, beziehen uns aber wieder auf SabeBacterium.
    **  Nur hier speichern wir den Eigentümer ab. Wir müssen beim
    **  Laden sowieso einen Refreshdurchlauf auf die gesamte
    **  Struktur machen, da fällt das mit ab.
    **
    **  Nicht alles wird beachtet, weil Neuinitialisierungen
    **  das Spielgeschehen nicht in jedem Fall durcheinander-
    **  bringen.
    **  -------------------------------------------------------*/
    char buffer[ 300 ];
    struct yparobo_data *yrd;

    /*** Eröffnung ***/
    sprintf( buffer, "\nbegin_robo %d\n", robo->bact->TypeID );
    _FWrite( buffer, strlen( buffer ), 1, SGF );

    /*** VehicleDaten ***/
    if( !yw_SaveBacterium( robo, SGF )) return( FALSE );

    /* ------------------------
    ** Nun wirds robospezifisch
    ** ----------------------*/

    /*** Allgemein ***/
    sprintf( buffer, "    owner          = %d\n\0", robo->bact->Owner );
    _FWrite( buffer, strlen( buffer ), 1, SGF );

    yrd = INST_DATA( ((struct nucleusdata *)robo->o)->o_Class, robo->o );

    /*** Vom RoboState nur das wichtige abspeichern ***/
    sprintf( buffer, "    robostate      = %d\n\0",
             (yrd->RoboState & ~(ROBO_BUILDRADAR  |ROBO_BUILDPOWER|
                                 ROBO_BUILDDEFENSE|ROBO_BUILDSAFETY|
                                 ROBO_BUILDCONQUER|ROBO_DEFENSEREADY|
                                 ROBO_POWERREADY  |ROBO_SAFETYREADY|
                                 ROBO_RADARREADY  |ROBO_CONQUERREADY)));
    _FWrite( buffer, strlen( buffer ), 1, SGF );

    /*** Das Dock erscheint mir noch wichtig ***/
    sprintf( buffer, "    dockenergy     = %ld\n\0", yrd->dock_energy );
    _FWrite( buffer, strlen( buffer ), 1, SGF );
    sprintf( buffer, "    dockcount      = %ld\n\0", yrd->dock_count );
    _FWrite( buffer, strlen( buffer ), 1, SGF );
    sprintf( buffer, "    dockuser       = %ld\n\0", yrd->dock_user );
    _FWrite( buffer, strlen( buffer ), 1, SGF );
    sprintf( buffer, "    docktime       = %ld\n\0", yrd->dock_time );
    _FWrite( buffer, strlen( buffer ), 1, SGF );
    sprintf( buffer, "    docktargetpos  = %2.2f_%2.2f\n\0",
             yrd->dtpos.x, yrd->dtpos.z );
    _FWrite( buffer, strlen( buffer ), 1, SGF );
    sprintf( buffer, "    docktargetID   = %ld\n\0", yrd->dtCommandID );
    _FWrite( buffer, strlen( buffer ), 1, SGF );
    sprintf( buffer, "    docktargettype = %d\n\0", yrd->dttype );
    _FWrite( buffer, strlen( buffer ), 1, SGF );
    sprintf( buffer, "    dockaggr       = %d\n\0", (LONG)yrd->dock_aggr );
    _FWrite( buffer, strlen( buffer ), 1, SGF );

    /*** UserBatterien ***/
    sprintf( buffer, "    battvehicle    = %ld\n\0", yrd->BattVehicle );
    _FWrite( buffer, strlen( buffer ), 1, SGF );
    sprintf( buffer, "    battbeam       = %ld\n\0", yrd->BattBeam );
    _FWrite( buffer, strlen( buffer ), 1, SGF );
    sprintf( buffer, "    fillmodus      = %d\n\0",  yrd->FillModus );
    _FWrite( buffer, strlen( buffer ), 1, SGF );
    sprintf( buffer, "    maximum        = %ld\n\0",  yrd->bact->Maximum );
    _FWrite( buffer, strlen( buffer ), 1, SGF );
    sprintf( buffer, "    buildspare     = %ld\n\0",  yrd->BuildSpare );
    _FWrite( buffer, strlen( buffer ), 1, SGF );

    /*** Derzeitige Viewerausrichtung ***/
    sprintf( buffer, "    vhoriz         = %7.5f\n\0",  yrd->bact->Viewer.horiz_angle );
    _FWrite( buffer, strlen( buffer ), 1, SGF );
    sprintf( buffer, "    vvert          = %7.5f\n\0",  yrd->bact->Viewer.vert_angle );
    _FWrite( buffer, strlen( buffer ), 1, SGF );

    /*** Die Prioritäten. Robos werden ja nicht aus LDF erzeugt ***/
    sprintf( buffer, "    con_budget     = %d\n\0",  yrd->ep_Conquer );
    _FWrite( buffer, strlen( buffer ), 1, SGF );
    sprintf( buffer, "    def_budget     = %d\n\0",  yrd->ep_Defense );
    _FWrite( buffer, strlen( buffer ), 1, SGF );
    sprintf( buffer, "    rec_budget     = %d\n\0",  yrd->ep_Reconnoitre );
    _FWrite( buffer, strlen( buffer ), 1, SGF );
    sprintf( buffer, "    rob_budget     = %d\n\0",  yrd->ep_Robo );
    _FWrite( buffer, strlen( buffer ), 1, SGF );
    sprintf( buffer, "    rad_budget     = %d\n\0",  yrd->ep_Radar );
    _FWrite( buffer, strlen( buffer ), 1, SGF );
    sprintf( buffer, "    pow_budget     = %d\n\0",  yrd->ep_Power );
    _FWrite( buffer, strlen( buffer ), 1, SGF );
    sprintf( buffer, "    saf_budget     = %d\n\0",  yrd->ep_Safety );
    _FWrite( buffer, strlen( buffer ), 1, SGF );
    sprintf( buffer, "    cpl_budget     = %d\n\0",  yrd->ep_ChangePlace );
    _FWrite( buffer, strlen( buffer ), 1, SGF );

    sprintf( buffer, "    saf_delay     = %d\n\0",  yrd->chk_Safety_Delay );
    _FWrite( buffer, strlen( buffer ), 1, SGF );
    sprintf( buffer, "    pow_delay     = %d\n\0",  yrd->chk_Power_Delay );
    _FWrite( buffer, strlen( buffer ), 1, SGF );
    sprintf( buffer, "    rad_delay     = %d\n\0",  yrd->chk_Radar_Delay );
    _FWrite( buffer, strlen( buffer ), 1, SGF );
    sprintf( buffer, "    cpl_delay     = %d\n\0",  yrd->chk_Place_Delay );
    _FWrite( buffer, strlen( buffer ), 1, SGF );
    sprintf( buffer, "    def_delay     = %d\n\0",  yrd->chk_Enemy_Delay );
    _FWrite( buffer, strlen( buffer ), 1, SGF );
    sprintf( buffer, "    con_delay     = %d\n\0",  yrd->chk_Terr_Delay );
    _FWrite( buffer, strlen( buffer ), 1, SGF );
    sprintf( buffer, "    rec_delay     = %d\n\0",  yrd->chk_Recon_Delay );
    _FWrite( buffer, strlen( buffer ), 1, SGF );
    sprintf( buffer, "    rob_delay     = %d\n\0",  yrd->chk_Robo_Delay );
    _FWrite( buffer, strlen( buffer ), 1, SGF );
    
    // FLOH: 22-Apr-98
    sprintf( buffer, "    reload_const  = %d\n\0",  yrd->bact->RoboReloadConst );
    _FWrite( buffer, strlen( buffer ), 1, SGF );      

    /*** Abschluß ***/
    sprintf( buffer, "end\n\n" );
    _FWrite( buffer, strlen( buffer ), 1, SGF );

    return( TRUE );
}


BOOL yw_SaveCommander( struct OBNode *commander, FILE *SGF )
{
    char buffer[ 300 ];

    /*** Eröffnung ***/
    sprintf( buffer, "\nbegin_commander %d\n", commander->bact->TypeID );
    _FWrite( buffer, strlen( buffer ), 1, SGF );

    /*** VehicleDaten ***/
    if( !yw_SaveBacterium( commander, SGF )) return( FALSE );
    if( !yw_SaveSpecials(  commander, SGF )) return( FALSE );

    /*** Abschluß ***/
    sprintf( buffer, "end\n\n" );
    _FWrite( buffer, strlen( buffer ), 1, SGF );

    return( TRUE );
}


BOOL yw_SaveSlave( struct OBNode *slave, FILE *SGF )
{
    char buffer[ 300 ];

    /*** Eröffnung ***/
    sprintf( buffer, "\nbegin_slave %d\n", slave->bact->TypeID );
    _FWrite( buffer, strlen( buffer ), 1, SGF );

    /*** VehicleDaten ***/
    if( !yw_SaveBacterium( slave, SGF )) return( FALSE );
    if( !yw_SaveSpecials(  slave, SGF )) return( FALSE );

    /*** Abschluß ***/
    sprintf( buffer, "end\n\n" );
    _FWrite( buffer, strlen( buffer ), 1, SGF );

    return( TRUE );
}


BOOL yw_SaveBacterium( struct OBNode *vehicle, FILE *SGF )
{
    /*  ---------------------------------------------------------
    **  Schreibt die Daten, die für jedes Vehicle gleich sind, in
    **  das File
    **  -------------------------------------------------------*/
    LONG    ATTR;
    char    buffer[ 300 ];
    int     i;

    /*** Viewer ***/
    _get( vehicle->o, YBA_Viewer, &ATTR );
    if( ATTR )
        sprintf( buffer, "    viewer         = yes\n\0");
    else
        sprintf( buffer, "    viewer         = no\n\0");
    _FWrite( buffer, strlen( buffer ), 1, SGF );

    /*** User ***/
    _get( vehicle->o, YBA_UserInput, &ATTR );
    if( ATTR )
        sprintf( buffer, "    user           = yes\n\0");
    else
        sprintf( buffer, "    user           = no\n\0");
    _FWrite( buffer, strlen( buffer ), 1, SGF );

    /*** Coll ***/
    _get( vehicle->o, YBA_BactCollision, &ATTR );
    if( ATTR )
        sprintf( buffer, "    collision      = yes\n\0");
    else
        sprintf( buffer, "    collision      = no\n\0");
    _FWrite( buffer, strlen( buffer ), 1, SGF );

    /*** physikalische Attribute ***/
    sprintf( buffer, "    energy         = %ld\n\0", vehicle->bact->Energy );
    _FWrite( buffer, strlen( buffer ), 1, SGF );

    sprintf( buffer, "    speed          = %6.5f_%6.5f_%6.5f_%6.5f\n\0",
             vehicle->bact->dof.x, vehicle->bact->dof.y,
             vehicle->bact->dof.z, vehicle->bact->dof.v );
    _FWrite( buffer, strlen( buffer ), 1, SGF );

    sprintf( buffer, "    matrix         = %6.5f_%6.5f_%6.5f_%6.5f_%6.5f_%6.5f_%6.5f_%6.5f_%6.5f\n\0",
             vehicle->bact->dir.m11, vehicle->bact->dir.m12, vehicle->bact->dir.m13,
             vehicle->bact->dir.m21, vehicle->bact->dir.m22, vehicle->bact->dir.m23,
             vehicle->bact->dir.m31, vehicle->bact->dir.m32, vehicle->bact->dir.m33 );
    _FWrite( buffer, strlen( buffer ), 1, SGF );

    /*** Sonderbehandlung bei position ***/
    if( vehicle->bact->BactClassID == BCLID_YPAROBO ) {

        struct yparobo_data *yrd;

        yrd = INST_DATA( ((struct nucleusdata *)vehicle->o)->o_Class, vehicle->o);
        sprintf( buffer, "    pos            = %2.2f_%2.2f_%2.2f\n\0",
                 vehicle->bact->pos.x, yrd->merke_y, vehicle->bact->pos.z );
        }
    else
        sprintf( buffer, "    pos            = %2.2f_%2.2f_%2.2f\n\0",
                 vehicle->bact->pos.x, vehicle->bact->pos.y, vehicle->bact->pos.z );
    _FWrite( buffer, strlen( buffer ), 1, SGF );

    sprintf( buffer, "    force          = %2.2f\n\0", vehicle->bact->act_force );
    _FWrite( buffer, strlen( buffer ), 1, SGF );

    sprintf( buffer, "    gunangle       = %5.4f\n\0", vehicle->bact->gun_angle_user );
    _FWrite( buffer, strlen( buffer ), 1, SGF );

    /*** Allgemeines ***/
    sprintf( buffer, "    commandID      = %ld\n\0", vehicle->bact->CommandID );
    _FWrite( buffer, strlen( buffer ), 1, SGF );

    sprintf( buffer, "    aggression     = %d\n\0", vehicle->bact->Aggression );
    _FWrite( buffer, strlen( buffer ), 1, SGF );

    sprintf( buffer, "    mainstate      = %d\n\0", vehicle->bact->MainState );
    _FWrite( buffer, strlen( buffer ), 1, SGF );

    sprintf( buffer, "    extrastate     = %ld\n\0", vehicle->bact->ExtraState );
    _FWrite( buffer, strlen( buffer ), 1, SGF );

    sprintf( buffer, "    ident          = %ld\n\0", vehicle->bact->ident );
    _FWrite( buffer, strlen( buffer ), 1, SGF );

    sprintf( buffer, "    killerowner    = %ld\n\0", vehicle->bact->killer_owner );
    _FWrite( buffer, strlen( buffer ), 1, SGF );

    /*** Ziele ***/
    if( TARTYPE_BACTERIUM == vehicle->bact->PrimTargetType )
        sprintf( buffer, "    primary        = %d_%ld_%2.2f_%2.2f_%ld\n\0",
                 vehicle->bact->PrimTargetType,
                 vehicle->bact->PrimaryTarget.Bact->ident,
                 vehicle->bact->PrimPos.x, vehicle->bact->PrimPos.z,
                 vehicle->bact->PrimCommandID  );
    else
        sprintf( buffer, "    primary        = %d_0_%2.2f_%2.2f_%ld\n\0",
                 vehicle->bact->PrimTargetType,
                 vehicle->bact->PrimPos.x, vehicle->bact->PrimPos.z,
                 vehicle->bact->PrimCommandID  );
    _FWrite( buffer, strlen( buffer ), 1, SGF );

    /*** Wegpunkte ***/
    for( i = 0; i < vehicle->bact->num_waypoints; i++ ) {

        sprintf( buffer, "    waypoint       = %d_%2.2f_%2.2f\n", i,
                 vehicle->bact->waypoint[ i ].x, vehicle->bact->waypoint[ i ].z );
        _FWrite( buffer, strlen( buffer ), 1, SGF );
        }
    sprintf( buffer, "    num_wp         = %d\n", vehicle->bact->num_waypoints );
    _FWrite( buffer, strlen( buffer ), 1, SGF );
    sprintf( buffer, "    count_wp       = %d\n", vehicle->bact->count_waypoints );
    _FWrite( buffer, strlen( buffer ), 1, SGF );


    return( TRUE );
}


BOOL yw_SaveSpecials( struct OBNode *vehicle, FILE *SGF )
{
    /*  -------------------------------------------------------
    **  Schreibt zusätzliche Daten, die in den LIDs, nicht aber
    **  in der bactstruktur stehen, in das File
    **  -----------------------------------------------------*/
    char    buffer[ 300 ];

    if( BCLID_YPAGUN == vehicle->bact->BactClassID ) {

        struct ypagun_data *ygd;

        /*** LID holen ***/
        ygd = INST_DATA( ((struct nucleusdata *)(vehicle->o))->o_Class, vehicle->o);

        /*** Basis rausschreiben ***/
        sprintf( buffer, "    gunbasis = %7.6f_%7.6f_%7.6f\n\0",
                 ygd->basis.x, ygd->basis.y, ygd->basis.z);
        _FWrite( buffer, strlen( buffer ), 1, SGF );
        }

    return( TRUE );
}


BOOL yw_SaveBuildInfo( struct ypaworld_data *ywd, FILE *SGF )
{
    /* ----------------------------------------------------------
    ** Speichert zuerst alle Vehiclebits und dann alle Baubits
    ** (also für die gebäude). Um den Parser vom Floh zu nehmen,
    ** speichere ich es nach seinen Richtlinien ab, auch wenn das
    ** 7-facher Platz bedeutet, aber soviel Vehicle gibt es ja
    ** nicht.
    ** --------------------------------------------------------*/

    int i;

    /*** Für die vehicle ***/
    for( i = 0; i < NUM_VEHICLEPROTOS; i++ ) {

        /*** Gibt es das Vehicle überhaupt? ***/
        if( ywd->VP_Array[ i ].BactClassID ) {

            char buffer[ 300 ];
            int  j;

            /*** Eröffnung schreiben ***/
            sprintf( buffer, "modify_vehicle %d\n\0", i );
            _FWrite( buffer, strlen( buffer ), 1, SGF );

            /*** je nach Eigentümer enable oder disable ***/
            for( j = 1; j < 8; j++ ) {

                if( ((UBYTE)(1<<j)) & ywd->VP_Array[ i ].FootPrint )
                    sprintf( buffer, "    enable         = %d\n\0", j );
                else
                    sprintf( buffer, "    disable        = %d\n\0", j );
                _FWrite( buffer, strlen( buffer ), 1, SGF );
                }

            /*** Rüstungsschutz,  max_energy, num_weapons ***/
            sprintf( buffer, "    shield         = %d\n\0", ywd->VP_Array[ i ].Shield );
            _FWrite( buffer, strlen( buffer ), 1, SGF );
            sprintf( buffer, "    energy         = %d\n\0", ywd->VP_Array[ i ].Energy );
            _FWrite( buffer, strlen( buffer ), 1, SGF );
            sprintf( buffer, "    num_weapons    = %d\n\0", ywd->VP_Array[ i ].NumWeapons );
            _FWrite( buffer, strlen( buffer ), 1, SGF );
            sprintf( buffer, "    weapon         = %d\n\0", ywd->VP_Array[ i ].Weapons[0] );
            _FWrite( buffer, strlen( buffer ), 1, SGF );
            sprintf( buffer, "    radar          = %d\n\0", ywd->VP_Array[ i ].View );
            _FWrite( buffer, strlen( buffer ), 1, SGF );
            sprintf( buffer, "    fire_x         = %4.2f\n\0", ywd->VP_Array[ i ].FireRelX );
            _FWrite( buffer, strlen( buffer ), 1, SGF );
            sprintf( buffer, "    fire_y         = %4.2f\n\0", ywd->VP_Array[ i ].FireRelY );
            _FWrite( buffer, strlen( buffer ), 1, SGF );
            sprintf( buffer, "    fire_z         = %4.2f\n\0", ywd->VP_Array[ i ].FireRelZ );
            _FWrite( buffer, strlen( buffer ), 1, SGF );

            /*** Abschluß ***/
            sprintf( buffer, "end\n\n\0");
            _FWrite( buffer, strlen( buffer ), 1, SGF );
            }
        }

    /*** Für die Waffen ***/
    for( i = 0; i < NUM_WEAPONPROTOS; i++ ) {

        /*** Gibt es das Vehicle überhaupt? ***/
        if( ywd->WP_Array[ i ].BactClassID ) {

            char buffer[ 300 ];
            int  j;

            /*** Eröffnung schreiben ***/
            sprintf( buffer, "modify_weapon %d\n\0", i );
            _FWrite( buffer, strlen( buffer ), 1, SGF );

            /*** je nach Eigentümer enable oder disable ***/
            for( j = 1; j < 8; j++ ) {

                if( ((UBYTE)(1<<j)) & ywd->WP_Array[ i ].FootPrint )
                    sprintf( buffer, "    enable         = %d\n\0", j );
                else
                    sprintf( buffer, "    disable        = %d\n\0", j );
                _FWrite( buffer, strlen( buffer ), 1, SGF );
                }

            /*** Rüstungsschutz,  max_energy, num_weapons ***/
            sprintf( buffer, "    shot_time      = %d\n\0", ywd->WP_Array[ i ].ShotTime );
            _FWrite( buffer, strlen( buffer ), 1, SGF );
            sprintf( buffer, "    shot_time_user = %d\n\0", ywd->WP_Array[ i ].ShotTime_User );
            _FWrite( buffer, strlen( buffer ), 1, SGF );
            sprintf( buffer, "    energy         = %d\n\0", ywd->WP_Array[ i ].Energy );
            _FWrite( buffer, strlen( buffer ), 1, SGF );
            
            /*** Abschluß ***/
            sprintf( buffer, "end\n\n\0");
            _FWrite( buffer, strlen( buffer ), 1, SGF );
            }
        }

    /*** Für die vehicle ***/
    for( i = 0; i < NUM_BUILDPROTOS; i++ ) {

        /*** Gibt es das Building. Hack. Ich brauch da mal genaue Infos ***/
        if( ywd->BP_Array[ i ].TypeIcon ) {

            char buffer[ 300 ];
            int  j;

            /*** Eröffnung schreiben ***/
            sprintf( buffer, "modify_building %d\n\0", i );
            _FWrite( buffer, strlen( buffer ), 1, SGF );

            /*** je nach Eigentümer enable oder disable ***/
            for( j = 1; j < 8; j++ ) {

                if( ((UBYTE)(1<<j)) & ywd->BP_Array[ i ].FootPrint )
                    sprintf( buffer, "    enable         = %d\n\0", j );
                else
                    sprintf( buffer, "    disable        = %d\n\0", j );
                _FWrite( buffer, strlen( buffer ), 1, SGF );
                }

            /*** Abschluß ***/
            sprintf( buffer, "end\n\n\0");
            _FWrite( buffer, strlen( buffer ), 1, SGF );
            }
        }

    return( TRUE );
}


BOOL yw_SaveWunderInfo( struct ypaworld_data *ywd, FILE *SGF )
{
    /* -----------------------------------------------------------
    ** Speichert alle Informationen zu Wundersteinen ab. Das ist
    ** zur zeit nur eine Disable-Sache, wenn der Wunderstein schon
    ** gelesen wurde
    ** ---------------------------------------------------------*/

    int  i;
    char buffer[ 300 ];

    /*** Eröffnung ***/
    sprintf( buffer, "\nbegin_wunderinfo\n\0");
    _FWrite( buffer, strlen( buffer ), 1, SGF );

    /*** Alle Wundersteine überprüfen ***/
    for( i = 0; i < MAXNUM_WUNDERSTEINS; i++ ) {

        struct Wunderstein *gem;

        /*** Wunderstein-Struktur holen ***/
        gem = &( ywd->gem[ i ]);

        /*** ok? ***/
        if( gem->active ) {

            struct Cell *sector;

            /*** Sektor dazu ermitteln ***/
            sector = &(ywd->CellArea[ gem->sec_x + gem->sec_y * ywd->MapSizeX ]);

            if( sector->WType != WTYPE_Wunderstein ) {

                /* --------------------------------------------------
                ** auf einem Wunderstein-Sektor kein Wunderstein? Das
                ** kann nur heißen: Bereits gefunden!
                ** ------------------------------------------------*/
                sprintf( buffer, "    disablegem %d\n\0", i );
                _FWrite( buffer, strlen( buffer ), 1, SGF );
                }
            }
        }

    /*** Abschluß ***/
    sprintf( buffer, "end\n\n\0");
    _FWrite( buffer, strlen( buffer ), 1, SGF );

    return( TRUE );
}


BOOL yw_SaveKWFactor( struct ypaworld_data *ywd, FILE *SGF )
{
    /* -------------------------------------------------------------
    ** Speichert alle Energyfaktoren der kraftwerke ab, diese unter-
    ** liegen ja Änderungen. Dazu scanne ich das Kraftwerk-Array
    ** der Welt. Ja, was machen eigentlich Kraftwerk?
    ** Diese Infos dürfen aber erst nach dem bauen der Kraftwerke
    ** gescannt werden!
    ** -----------------------------------------------------------*/

    int  i;
    char buffer[ 300 ];

    /*** Eröffnung ***/
    sprintf( buffer, "\nbegin_kwfactor\n\0");
    _FWrite( buffer, strlen( buffer ), 1, SGF );

    /*** Alle Wundersteine überprüfen ***/
    for( i = 0; i < MAXNUM_KRAFTWERKS; i++ ) {

        struct KraftWerk *kw;

        /*** Wunderstein-Struktur holen ***/
        kw = &(ywd->KraftWerks[ i ]);

        /*** Ausgefüllt? ***/
        if( kw->sector ) {

            struct Cell *sector;

            /*** Sektor dazu ermitteln ***/
            sector = &(ywd->CellArea[ kw->x + kw->y * ywd->MapSizeX ]);

            /*** Mit Position abspeichern ***/

            sprintf( buffer, "    kw = %d_%d_%ld\n\0", kw->x, kw->y, kw->factor );
            _FWrite( buffer, strlen( buffer ), 1, SGF );
            }
        }

    /*** Abschluß ***/
    sprintf( buffer, "end\n\n\0");
    _FWrite( buffer, strlen( buffer ), 1, SGF );

    return( TRUE );
}


BOOL yw_SaveGlobals( struct ypaworld_data *ywd, FILE *SGF )
{
    /* -------------------------------------------------------------
    ** -----------------------------------------------------------*/

    char buffer[ 300 ];

    /*** Eröffnung ***/
    sprintf( buffer, "\nbegin_globals\n\0");
    _FWrite( buffer, strlen( buffer ), 1, SGF );


    sprintf( buffer, "    time = %ld\n", ywd->TimeStamp );
    _FWrite( buffer, strlen( buffer ), 1, SGF );

    /*** Abschluß ***/
    sprintf( buffer, "end\n\n\0");
    _FWrite( buffer, strlen( buffer ), 1, SGF );

    return( TRUE );
}


/*  -----------------------------------------------------------------
**
**  D I E  L E V E L S P E Z I F I S C H E N  L A D E R O U T I N E N
**
**  ---------------------------------------------------------------*/


_dispatcher( BOOL, yw_YWM_LOADGAME, struct saveloadgame_msg *slg )
{
/*
**  FUNCTION    Frontend für das Laden eines levels. Oje, das wird
**              ein Hammer. Also zuerst parse ich die Scripts und
**              baue damit die Geschwaderstruktur auf. Weil die ID
**              schon am Anfang steht, kann ich gleich ein Objekt
**              erzeugen und ausfüllen.
**              dann gehen die Initorgien los...
**
**              NEU: Es wird zur modifzierten Kopie von CreateLevel.
**              lediglich die Objekte werden wegggelassen und eigenständig
**              erzeugt und nachbearbeitet. Gebäude werden ohne Robo
**              vehiclelos erzeugt.
**
**              evtl. Cancel-Mauszeiger
**
*/
    struct ypaworld_data *ywd;
    char   filename[ 300 ];
    struct LevelDesc ld;
    BOOL   level_ok = FALSE;
    ULONG  secx, secy, lnum;

    ywd = INST_DATA( cl, o );

    /* ----------------------------------------------------------------
    ** Bevor wir den level killen, sehen wir nach, ob zu dem Level alle
    ** Files existieren, sonst gibts Ärger. Ich gebe aber TRUE zurück,
    ** damit er nicht abbricht, denn FALSE heißt Fehler!
    ** --------------------------------------------------------------*/
    if( !yw_ExistFileName( slg->name ) )
        return( TRUE );

    /* ----------------------------------------------------------------------
    ** bei normalen #?.sgm-Files dürfen wir nicht maximum auf MaxRoboenergy
    ** setzen. Deshalb löschen wir es einfach vorher. Beim nächsten verlassen
    ** durch ein Gate wird es ja sowieso wieder aktualisiert.
    ** --------------------------------------------------------------------*/
    if( strstr( slg->name, ".SGM") || strstr( slg->name, ".sgm") ) {
        ywd->MaxRoboEnergy  = 0;
        ywd->MaxReloadConst = 0;
    };

    /*** Filename ermitteln ***/
    sprintf( filename, "%s\0", slg->name );

    /*** Levelnummer ermitteln, ist in jedem File! ***/
    yw_ParseLevelNumber2( filename, &lnum );

    /*** globale Variable rücksetzen ***/
    EXTRA_VIEWER  = EV_No;
    VIEWER_NUMBER = -1;

    // FIXME_FLOH: yw_CommonLevelInit() + Local Vars aufgeräumt!!!
    if (yw_CommonLevelInit(ywd,&ld,lnum,LEVELMODE_NORMAL)) {
        if (yw_LoadTypMap(ywd,ld.typ_map)) {
            if (yw_LoadOwnMap(ywd,ld.own_map)) {
                if (yw_LoadHgtMap(ywd,ld.hgt_map)) {
                    if (yw_LoadBlgMap(o,ywd,ld.blg_map)) {   // ??? wieso LBM, wenn noch keine Robos da sind???
                        level_ok = TRUE;
                    };
                };
            };
        };
    };

    if (!level_ok) {
        _methoda(o, YWM_KILLLEVEL, NULL);
        return( FALSE );
        }


    /*** Zähler: Vorarbeit ***/
    DeathCount   = (1<<16);
    CommandCount = 0;
    ViewerRobo   = NULL;
    ywd->Level->OwnerMask = 0;       // Wird dann von Parse aktualisiert
    ywd->Level->UserMask  = 0;

    /*** muss vor dem Laden initialisieren ***/
    yw_InitSuperItems(ywd);

    /*** Backup von originaler Owner und TypeMap ***/
    if (ywd->TypeMapBU) {
        _dispose(ywd->TypeMapBU);
        ywd->TypeMapBU = NULL;
    };
    if (ywd->OwnerMapBU) {
        _dispose(ywd->OwnerMapBU);
        ywd->OwnerMapBU = NULL;
    };
    if (ywd->TypeMap) {
        ywd->TypeMapBU = yw_CopyBmpObject(ywd->TypeMap,"copyof_typemap");
    };
    if (ywd->OwnerMap) {
        ywd->OwnerMapBU = yw_CopyBmpObject(ywd->OwnerMap,"copyof_ownermap");
    };

    /*** Vehicle reinholen und Wunderinfos lesen ***/
    if( !yw_ParseVehicles( ywd, filename ) ) return( FALSE );

    /*** Zähler: Nacharbeit ***/
    DeathCount++;
    CommandCount++;
    if( ViewerRobo )
        _set( ViewerRobo, YRA_CommandCount, CommandCount );

    /*** Vehiclestruktur reorganisieren ***/
    yw_ReorganizeVehicles( ywd );

    /* --------------------------------------------------------------
    ** Wenn was im Topf war, dann ausschütten, weil "LOAD Final" aus-
    ** sieht, als wäre es ein CREATELEVEL
    ** ------------------------------------------------------------*/
    if( strstr( slg->name, ".fin" ) ||
        strstr( slg->name, ".FIN" ))
        yw_CreateBuddies( ywd );

    /* -------------------------------------------------------------
    ** globaler Sector-Owner-Check
    ** Dieser und NewEnergyCycle werden jetzt hier gemacht, weil die
    ** Maps erst bei ParseVehicles ausgewertet werden...
    ** -----------------------------------------------------------*/
    for (secy=0; secy<ywd->MapSizeY; secy++) {
        for (secx=0; secx<ywd->MapSizeX; secx++) {
            struct Cell *sec;
            sec = &(ywd->CellArea[secy*ywd->MapSizeX + secx]);
            // FIXME_FLOH
            yw_CheckSector(ywd,sec,secx,secy,255,NULL);
        };
    };

    /*** BeamGates initialisieren ***/
    yw_InitGates(ywd);

    /*** More Energy-Init ***/
    yw_NewEnergyCycle(ywd);

    /*** GUI-Modul initialisieren ***/
    if( !yw_InitGUIModule( o, ywd )) return( FALSE );

    return( TRUE );
}


BOOL yw_ParseLevelNumber2( char *filename, ULONG *lnum )
{
    /* ------------------------------------------------------------
    ** Parst einen File und sucht den Block begin_levlnum. Dazu
    ** gibt es zwar einen Parser, ich muß aber dies auch ermitteln,
    ** ohne den Rest, dann bringt der Parser aber Fehler. Deshalb
    ** nehme ich einen eigenen "Single-Parser". Ich mache das etwas
    ** allgemeiner, weil evtl. auch andere Sachen in diesem Block
    ** stehen können.
    ** ----------------------------------------------------------*/
    FILE *f;
    char line[ 255 ], *p;

    if( f = _FOpen( filename, "r" ) ) {

        while( fgets( line, 255, f )  ) {

            if( strnicmp( line, "begin_levelnum", 14 ) == 0 ) {

                /*** inner nächsten Zeile isses ***/
                while( fgets( line, 255, f ) ) {

                    /*** Ende? ***/
                    if( strnicmp( line, "end", 3 ) == 0 )
                        return( FALSE );    // weil nix gefunden

                    /*** Das keyword ***/
                    if( p = strtok( line, " \t" ) ) {

                        if( strnicmp( p, "levelnum", 8 ) == 0 ) {

                            /*** Die daten ***/
                            if( p = strtok( NULL, " \t=" ) ) {

                                /*** Auswerten und raus ***/
                                *lnum = atol( p );
                                _FClose( f );
                                return( TRUE );
                                }
                            }
                        }
                    }
                }
            }

        _FClose( f );
        }

    return( FALSE );
}


ULONG yw_ParseVehicles( struct ypaworld_data *ywd, char *filename )
{
    /* -------------------------------------------------------
    ** FrontEnd für die (trotz des Namens ALLE) ParserRoutinen
    ** NEU: Buddies werden nicht mehr behandelt.
    ** -----------------------------------------------------*/
    ULONG p = 0, ln;
    struct ScriptParser parser[19];
    memset(parser, 0, sizeof(parser));

    parser[p].parse_func = yw_ParseUserData;
    parser[p].store[0]   = ywd->world;
    parser[p].store[1]   = ywd;
    p++;
    
    parser[p].parse_func = yw_ParseRobo;
    parser[p].target     = ywd;
    p++;

    parser[p].parse_func = yw_ParseCommander;
    parser[p].target     = ywd;
    p++;

    parser[p].parse_func = yw_ParseSlave;
    parser[p].target     = ywd;
    p++;

    parser[p].parse_func = yw_ParseWunderInfo;
    parser[p].target     = ywd;
    p++;

    parser[p].parse_func = yw_VhclProtoParser;  // für bauflags, energy, shield...
    parser[p].store[0]   = (ULONG) ywd;
    p++;

    parser[p].parse_func = yw_WeaponProtoParser;  // für bauflags und anderes
    parser[p].store[0]   = (ULONG) ywd;
    p++;

    parser[p].parse_func = yw_BuildProtoParser;  // für bauflags
    parser[p].store[1]   = (ULONG) ywd;
    p++;

    parser[p].parse_func = yw_ParseExtraViewerInfo;  //
    p++;

    parser[p].parse_func = yw_ParseKWFactor;  //
    parser[p].target     = ywd;
    p++;

    parser[p].parse_func = yw_ParseGlobals;  // für Zeuch und so...
    parser[p].target     = ywd;
    p++;

    parser[p].parse_func = yw_ParseOwnerMap;  //
    parser[p].target     = ywd;
    p++;

    parser[p].parse_func = yw_ParseBuildingMap;  //
    parser[p].target     = ywd;
    p++;

    parser[p].parse_func = yw_ParseEnergyMap;  //
    parser[p].target     = ywd;
    p++;

    parser[p].parse_func = yw_ParseLevelNum;  // damit kein Fehler kommt
    parser[p].target     = &ln;
    p++;

    parser[p].parse_func = yw_ParseLevelStatus;  // damit kein Fehler kommt
    parser[p].target     = ywd;
    p++;

    parser[p].parse_func = yw_HistoryParser;
    parser[p].target     = ywd;
    p++;

    parser[p].parse_func = yw_ParseMasks;
    parser[p].target     = ywd;
    p++;

    parser[p].parse_func = yw_ParseSuperBombs;
    parser[p].target     = ywd;
    p++;

    /*** nu los ***/
    return( yw_ParseScript( filename, p, parser, PARSEMODE_EXACT ) );
}


ULONG yw_ParseLevelNum( struct ScriptParser *parser )
{
    /*** Parst die Levelnummer und nur die ***/

    if( PARSESTAT_READY == parser->status ) {

        /*** Wir sind bereit für etwas neues ***/
        if( stricmp( parser->keyword, "begin_levelnum") == 0 ) {

            parser->status = PARSESTAT_RUNNING;
            return( PARSE_ENTERED_CONTEXT );
            }
        else {

            /* -----------------------------------------------------
            ** Ich muß den Parser machen wie alle anderen auch, weil
            ** er sonst mit anderen nicht zurechtkommt
            ** ---------------------------------------------------*/
            return( PARSE_UNKNOWN_KEYWORD );
            }
        }
    else {

        /*** Wir bearbeiten das schon. Ist es das Ende? ***/
        if( stricmp( parser->keyword, "end") == 0 ) {

            /*** Das Ende naht! ***/
            parser->status = PARSESTAT_READY;
            return( PARSE_LEFT_CONTEXT );
            }
        else {

            if( stricmp( parser->keyword, "levelnum" ) == 0 ) {

                *( (ULONG *)(parser->target) ) = atol( parser->data );
                return( PARSE_ALL_OK );
                }

            return( PARSE_UNKNOWN_KEYWORD );
            }
        }
}


ULONG yw_ParseSuperBombs( struct ScriptParser *parser )
{
    /*** Parst die Levelnummer und nur die ***/
    struct ypaworld_data *ywd;                                            

    ywd = (struct ypaworld_data *) parser->target;

    if( PARSESTAT_READY == parser->status ) {

        /*** Wir sind bereit für etwas neues ***/
        if( stricmp( parser->keyword, "begin_superbomb") == 0 ) {

            parser->status = PARSESTAT_RUNNING;
            return( PARSE_ENTERED_CONTEXT );
            }
        else {

            /* -----------------------------------------------------
            ** Ich muß den Parser machen wie alle anderen auch, weil
            ** er sonst mit anderen nicht zurechtkommt
            ** ---------------------------------------------------*/
            return( PARSE_UNKNOWN_KEYWORD );
            }
        }
    else {

        /*** Wir bearbeiten das schon. Ist es das Ende? ***/
        if( stricmp( parser->keyword, "end") == 0 ) {

            /*** Das Ende naht! ***/
            parser->status = PARSESTAT_READY;
            return( PARSE_LEFT_CONTEXT );
            }
        else {

            if( stricmp( parser->keyword, "status" ) == 0 ) {

                /*** Da machen wir gleich mal ne Initialisierung ***/
                LONG status;
                status = atol( parser->data );

                switch( status ) {

                    case SI_STATUS_ACTIVE:
                        yw_InitActiveItem( ywd, superitem, FALSE );
                        break;

                    case SI_STATUS_FROZEN:
                        yw_InitFreezeItem( ywd, superitem );
                        break;

                    case SI_STATUS_TRIGGERED:
                        yw_InitTriggerItem( ywd, superitem );
                        break;
                    }
                return( PARSE_ALL_OK );
                }

            if( stricmp( parser->keyword, "num" ) == 0 ) {

                superitem = atol( parser->data );
                return( PARSE_ALL_OK );
                }

            if( stricmp( parser->keyword, "activated_by" ) == 0 ) {

                ywd->Level->Item[ superitem ].activated_by = atol(parser->data);
                return( PARSE_ALL_OK );
                }

            if( stricmp( parser->keyword, "active_timestamp" ) == 0 ) {

                ywd->Level->Item[ superitem ].active_timestamp = atol(parser->data);
                return( PARSE_ALL_OK );
                }

            if( stricmp( parser->keyword, "trigger_timestamp" ) == 0 ) {

                ywd->Level->Item[ superitem ].trigger_timestamp = atol(parser->data);
                return( PARSE_ALL_OK );
                }

            if( stricmp( parser->keyword, "countdown" ) == 0 ) {

                ywd->Level->Item[ superitem ].countdown = atol(parser->data);
                return( PARSE_ALL_OK );
                }

            if( stricmp( parser->keyword, "last_ten_sec" ) == 0 ) {

                ywd->Level->Item[ superitem ].last_ten_sec = atol(parser->data);
                return( PARSE_ALL_OK );
                }

            if( stricmp( parser->keyword, "last_sec" ) == 0 ) {

                ywd->Level->Item[ superitem ].last_sec = atol(parser->data);
                return( PARSE_ALL_OK );
                }

            if( stricmp( parser->keyword, "radius" ) == 0 ) {

                ywd->Level->Item[ superitem ].radius = atol(parser->data);
                return( PARSE_ALL_OK );
                }

            if( stricmp( parser->keyword, "last_radius" ) == 0 ) {

                ywd->Level->Item[ superitem ].last_radius = atol(parser->data);
                return( PARSE_ALL_OK );
                }

            return( PARSE_UNKNOWN_KEYWORD );
            }
        }
}


ULONG yw_ParseMasks( struct ScriptParser *parser )
{
    /*** Parst die Levelnummer und nur die ***/
    struct ypaworld_data *ywd;                                            

    ywd = (struct ypaworld_data *) parser->target;

    if( PARSESTAT_READY == parser->status ) {

        /*** Wir sind bereit für etwas neues ***/
        if( stricmp( parser->keyword, "begin_masks") == 0 ) {

            parser->status = PARSESTAT_RUNNING;
            return( PARSE_ENTERED_CONTEXT );
            }
        else {

            /* -----------------------------------------------------
            ** Ich muß den Parser machen wie alle anderen auch, weil
            ** er sonst mit anderen nicht zurechtkommt
            ** ---------------------------------------------------*/
            return( PARSE_UNKNOWN_KEYWORD );
            }
        }
    else {

        /*** Wir bearbeiten das schon. Ist es das Ende? ***/
        if( stricmp( parser->keyword, "end") == 0 ) {

            /*** Das Ende naht! ***/
            parser->status = PARSESTAT_READY;
            return( PARSE_LEFT_CONTEXT );
            }
        else {

            if( stricmp( parser->keyword, "ownermask" ) == 0 ) {

                ywd->Level->OwnerMask = atol( parser->data );
                return( PARSE_ALL_OK );
                }

            if( stricmp( parser->keyword, "usermask" ) == 0 ) {

                ywd->Level->UserMask = atol( parser->data );
                return( PARSE_ALL_OK );
                }

            return( PARSE_UNKNOWN_KEYWORD );
            }
        }
}


ULONG yw_ParseOwnerMap( struct ScriptParser *parser )
{
    /*** Parst die Energy-faktoren für die Kraftwerke ***/
    struct ypaworld_data *ywd;                                            

    if( parser->target ) ywd = parser->target;
    
    if( PARSESTAT_READY == parser->status ) {

        /*** Wir sind bereit für etwas neues ***/
        if( stricmp( parser->keyword, "begin_ownermap") == 0 ) {

            ULONG i;
            struct Cell *sector;
            struct VFMBitmap *bm;
            UBYTE  *owner;

            /*** Das ist etwas für uns ***/
            parser->status = PARSESTAT_RUNNING;

            /*** Sector-Counter auf 0 ***/
            memset(&(ywd->SectorCount),0,sizeof(ywd->SectorCount));

            /*** Itze den ganzen Block parsen ***/
            if( ywd->OwnerMap ) _dispose( ywd->OwnerMap );
            ywd->OwnerMap = yw_CreateBmpFromAscii(ywd,"ownmap",parser->fp);
            if (NULL == ywd->OwnerMap) return(PARSE_BOGUS_DATA);

            /*** Rückschreiben in Sektorstrukturen ***/
            _get( ywd->OwnerMap, BMA_Bitmap, &bm );

            /*** Daten umkopieren ***/
            owner  = (UBYTE *)(bm->Data);
            sector = ywd->CellArea;
            for( i = 0; i < (ywd->MapSizeX * ywd->MapSizeY); i++ ) {

                sector[ i ].Owner = owner[ i ];
                ywd->SectorCount[ owner[i] ]++;
                }

            return( PARSE_ENTERED_CONTEXT );
            }
        else {

            /*** Damit kann ich nichts anfangen ***/
            return( PARSE_UNKNOWN_KEYWORD );
            }
        }
    else {

        /*** Wir bearbeiten das schon. Ist es das Ende? ***/
        if( stricmp( parser->keyword, "end") == 0 ) {

            /*** Das Ende naht! ***/
            parser->status = PARSESTAT_READY;
            return( PARSE_LEFT_CONTEXT );
            }
        else {

            return( PARSE_UNKNOWN_KEYWORD );
            }
        }
}


ULONG yw_ParseBuildingMap( struct ScriptParser *parser )
{
    /*** Parst die Energy-faktoren für die Kraftwerke ***/
    struct ypaworld_data *ywd;                                            

    if( parser->target ) ywd = parser->target;
    
    if( PARSESTAT_READY == parser->status ) {

        /*** Wir sind bereit für etwas neues ***/
        if( stricmp( parser->keyword, "begin_buildingmap") == 0 ) {

            ULONG i;
            struct Cell *sector;
            struct VFMBitmap *bm;
            UBYTE  *building;

            /*** Das ist etwas für uns ***/
            parser->status = PARSESTAT_RUNNING;

            /*** Itze den ganzen Block parsen ***/
            if( ywd->BuildMap ) _dispose( ywd->BuildMap );
            ywd->BuildMap = yw_CreateBmpFromAscii(ywd,"blgmap",parser->fp);
            if (NULL == ywd->BuildMap) return(PARSE_BOGUS_DATA);

            /*** Rückschreiben in Sektorstrukturen ***/
            _get( ywd->BuildMap, BMA_Bitmap, &bm );

            /*** Daten umkopieren ***/
            building = (UBYTE *)(bm->Data);
            sector   = ywd->CellArea;
            for( i = 0; i < (ywd->MapSizeX * ywd->MapSizeY); i++ ) {

                if ((building[i] != 0) && (sector[i].Owner != 0)) {

                    struct createbuilding_msg cv_msg;

                    /* ----------------------------------------------------
                    ** Grundsätzlich ohne Vehicle, da die durch die normale
                    ** Geschwaderstruktur definiert sind!!!
                    ** --------------------------------------------------*/
                    cv_msg.flags     = CBF_NOBACTS;
                    cv_msg.job_id    = sector[i].Owner;
                    cv_msg.owner     = sector[i].Owner;
                    cv_msg.bp        = building[i];
                    cv_msg.immediate = TRUE;
                    cv_msg.sec_x     = i % ywd->MapSizeX;
                    cv_msg.sec_y     = i / ywd->MapSizeX;
                    _methoda(ywd->world, YWM_CREATEBUILDING, &cv_msg);
                    }
                }

            return( PARSE_ENTERED_CONTEXT );
            }
        else {

            /*** Damit kann ich nichts anfangen ***/
            return( PARSE_UNKNOWN_KEYWORD );
            }
        }
    else {

        /*** Wir bearbeiten das schon. Ist es das Ende? ***/
        if( stricmp( parser->keyword, "end") == 0 ) {

            /*** Das Ende naht! ***/
            parser->status = PARSESTAT_READY;
            return( PARSE_LEFT_CONTEXT );
            }
        else {

            return( PARSE_UNKNOWN_KEYWORD );
            }
        }
}


ULONG yw_ParseEnergyMap( struct ScriptParser *parser )
{
    /*** Parst die Energy-faktoren für die Kraftwerke ***/
    struct ypaworld_data *ywd;                                            

    if( parser->target ) ywd = parser->target;
    
    if( PARSESTAT_READY == parser->status ) {

        /*** Wir sind bereit für etwas neues ***/
        if( stricmp( parser->keyword, "begin_energymap") == 0 ) {

            Object *BMO;
            struct VFMBitmap *bm;
            struct Cell *sector;
            UBYTE  *energy;
            int    i, j, k;

            /*** Das ist etwas für uns ***/
            parser->status = PARSESTAT_RUNNING;

            /*** BMO erzeugen ***/
            BMO = yw_CreateBmpFromAscii(ywd,"nrgmap",parser->fp);
            if( !BMO ) {

                _LogMsg("Error while reading energy map!\n");
                return( PARSE_BOGUS_DATA );
                }

            /*** Bitmap holen ***/
            _get( BMO, BMA_Bitmap, &bm );

            /*** Daten umkopieren ***/
            energy = (UBYTE *)(bm->Data);
            sector = ywd->CellArea;

            for( i = 0; i < (ywd->MapSizeX * ywd->MapSizeY); i++ )
                for( j = 0; j < 3; j++ )
                    for( k = 0; k < 3; k++ )
                        sector[ i ].SubEnergy[ j ][ k ] = energy[ 9 * i + 3 * j + k ];

            /*** Das woll mr mol net vergessn ***/
            _dispose( BMO );

            return( PARSE_ENTERED_CONTEXT );
            }
        else {

            /*** Damit kann ich nichts anfangen ***/
            return( PARSE_UNKNOWN_KEYWORD );
            }
        }
    else {

        /*** Wir bearbeiten das schon. Ist es das Ende? ***/
        if( stricmp( parser->keyword, "end") == 0 ) {

            /*** Das Ende naht! ***/
            parser->status = PARSESTAT_READY;
            return( PARSE_LEFT_CONTEXT );
            }
        else {

            return( PARSE_UNKNOWN_KEYWORD );
            }
        }
}


ULONG yw_ParseGlobals( struct ScriptParser *parser )
{
    /*** Parst die Energy-faktoren für die Kraftwerke ***/
    struct ypaworld_data *ywd;                                            

    if( parser->target ) ywd = parser->target;
    
    if( PARSESTAT_READY == parser->status ) {

        /*** Wir sind bereit für etwas neues ***/
        if( stricmp( parser->keyword, "begin_globals") == 0 ) {

            /*** Das ist etwas für uns ***/
            parser->status = PARSESTAT_RUNNING;
            return( PARSE_ENTERED_CONTEXT );
            }
        else {

            /*** Damit kann ich nichts anfangen ***/
            return( PARSE_UNKNOWN_KEYWORD );
            }
        }
    else {

        /*** Wir bearbeiten das schon. Ist es das Ende? ***/
        if( stricmp( parser->keyword, "end") == 0 ) {

            /*** Das Ende naht! ***/
            parser->status = PARSESTAT_READY;
            return( PARSE_LEFT_CONTEXT );
            }
        else {

            /*** Es sollte also ein  normales keyword sein ***/

            if( stricmp( parser->keyword, "time" ) == 0 ) {

                /* ------------------------------------------------
                ** Die global_time. Wir müssen die in der trigger-
                ** Message setzen, weil diese dort die aktuelle und
                ** einzig wahre ist.
                ** ----------------------------------------------*/
                ywd->TimeStamp = atol( parser->data );
                }
            else {
                return( PARSE_UNKNOWN_KEYWORD );
                }

            return( PARSE_ALL_OK );
            }
        }
}



ULONG yw_ParseKWFactor( struct ScriptParser *parser )
{
    /*** Parst die Energy-faktoren für die Kraftwerke ***/
    struct ypaworld_data *ywd;                                            

    if( parser->target ) ywd = parser->target;
    
    if( PARSESTAT_READY == parser->status ) {

        /*** Wir sind bereit für etwas neues ***/
        if( stricmp( parser->keyword, "begin_kwfactor") == 0 ) {

            /*** Das ist etwas für uns ***/
            parser->status = PARSESTAT_RUNNING;
            return( PARSE_ENTERED_CONTEXT );
            }
        else {

            /*** Damit kann ich nichts anfangen ***/
            return( PARSE_UNKNOWN_KEYWORD );
            }
        }
    else {

        /*** Wir bearbeiten das schon. Ist es das Ende? ***/
        if( stricmp( parser->keyword, "end") == 0 ) {

            /*** Das Ende naht! ***/
            parser->status = PARSESTAT_READY;
            return( PARSE_LEFT_CONTEXT );
            }
        else {

            /*** Es sollte also ein  normales keyword sein ***/

            if( stricmp( parser->keyword, "kw" ) == 0 ) {

                /* -----------------------------------------------------
                ** Soso, ein Faktor. Wir parsen die Sachen, erhalten die
                ** Position, gucken, ob die existiert und schreiben den
                ** Factor rein, ja genau so!
                ** ---------------------------------------------------*/
                WORD  sec_x, sec_y;
                ULONG factor;
                char  *ptr;

                if( ptr = strtok( parser->data, "_ \n" ) ) {

                    sec_x = atoi( ptr );
                    if( ptr = strtok( NULL, "_ \n" ) ) {

                        sec_y = atoi( ptr );
                        if( ptr = strtok( NULL, "_ \n" ) ) {

                            int i;
                            factor = atoi( ptr );

                            /* ----------------------------------
                            ** Nu hammer alle Infos, also gucken,
                            ** ob das Ding existiert
                            ** --------------------------------*/
                            for( i = 0; i < MAXNUM_KRAFTWERKS; i++ ) {

                                if( ywd->KraftWerks[ i ].sector ) {

                                    if( (ywd->KraftWerks[ i ].x == sec_x) &&
                                        (ywd->KraftWerks[ i ].y == sec_y) )
                                        ywd->KraftWerks[ i ].factor = factor;
                                    }
                                }
                            }
                        }
                    }
                }
            else {
                return( PARSE_UNKNOWN_KEYWORD );
                }

            return( PARSE_ALL_OK );
            }
        }
}


ULONG yw_ParseExtraViewerInfo( struct ScriptParser *parser )
{
    /*** Sucht nach eventuellen zusätzlichen Viewerinfos ***/
    
    if( PARSESTAT_READY == parser->status ) {

        /*** Wir sind bereit für etwas neues ***/
        if( stricmp( parser->keyword, "begin_extraviewer") == 0 ) {

            /*** Das ist etwas für uns ***/
            parser->status = PARSESTAT_RUNNING;
            return( PARSE_ENTERED_CONTEXT );
            }
        else {

            /*** Damit kann ich nichts anfangen ***/
            return( PARSE_UNKNOWN_KEYWORD );
            }
        }
    else {

        /*** Wir bearbeiten das schon. Ist es das Ende? ***/
        if( stricmp( parser->keyword, "end") == 0 ) {

            /*** Das Ende naht! ***/
            parser->status = PARSESTAT_READY;
            return( PARSE_LEFT_CONTEXT );
            }
        else {

            /*** Es sollte also ein  normales keyword sein ***/

            if( stricmp( parser->keyword, "kind" ) == 0 ) {

                if( stricmp( parser->data, "robogun" ) == 0 )
                    EXTRA_VIEWER = EV_RoboGun;
                }
            else {

            if( stricmp( parser->keyword, "number" ) == 0 ) {

                VIEWER_NUMBER = atol( parser->data );
                }
            else {
                return( PARSE_UNKNOWN_KEYWORD );
                } }

            return( PARSE_ALL_OK );
            }
        }
}


ULONG yw_ParseWunderInfo( struct ScriptParser *parser )
{
    /*** Parst die DisableInfos zu den Wundersteinen ***/
    struct ypaworld_data *ywd;                                            

    if( parser->target ) ywd = parser->target;

    if( PARSESTAT_READY == parser->status ) {

        /*** Wir sind bereit für etwas neues ***/
        if( stricmp( parser->keyword, "begin_wunderinfo") == 0 ) {

        /*** Wundersteine initialisieren ***/
        yw_InitWundersteins(ywd->world,ywd);

            /*** Das ist etwas für uns ***/
            parser->status = PARSESTAT_RUNNING;
            return( PARSE_ENTERED_CONTEXT );
            }
        else {

            /*** Damit kann ich nichts anfangen ***/
            return( PARSE_UNKNOWN_KEYWORD );
            }
        }
    else {

        /*** Wir bearbeiten das schon. Ist es das Ende? ***/
        if( stricmp( parser->keyword, "end") == 0 ) {

            /*** Das Ende naht! ***/
            parser->status = PARSESTAT_READY;
            return( PARSE_LEFT_CONTEXT );
            }
        else {

            /*** Es sollte also ein  normales keyword sein ***/
            if( parser->target ) {

                if( stricmp( parser->keyword, "disablegem" ) == 0 ) {

                    /*** Den Wunderstein disablen ***/
                    LONG num = atol( parser->data );
                    if( (num >= 0) && (num < MAXNUM_WUNDERSTEINS) ) {

                        /* --------------------------------------------
                        ** Sektor holen und evtl. bereits geschriebenen
                        ** WType löschen
                        ** ------------------------------------------*/
                        struct Wunderstein *gem;
                        struct Cell *sector;

                        gem = &(ywd->gem[ num ]);
                        sector = &(ywd->CellArea[ gem->sec_x +
                                                  gem->sec_y * ywd->MapSizeX ]);

                        sector->WType = WTYPE_None;
                        }
                    }
                else
                    return( PARSE_UNKNOWN_KEYWORD );
                }

            return( PARSE_ALL_OK );
            }
        }
}


ULONG yw_ParseSlave( struct ScriptParser *parser )
{
    struct ypaworld_data *ywd;                                            

    if( parser->target ) ywd = parser->target;

    if( PARSESTAT_READY == parser->status ) {

        /*** Wir sind bereit für etwas neues ***/
        if( stricmp( parser->keyword, "begin_slave") == 0 ) {

            struct createvehicle_msg cv;

            /*** Das ist etwas für uns ***/
            parser->status = PARSESTAT_RUNNING;

            /* ----------------------------------------------------------
            ** in data steht jetzt die ID, wir erzeugen gleich ein Objekt
            ** dazu, welches wir später richtig positionieren
            ** --------------------------------------------------------*/
            cv.x    =  SECTOR_SIZE / 2;
            cv.z    = -SECTOR_SIZE / 2;
            cv.y    =  0.0;
            cv.vp   =  atoi( parser->data );
            if( !( ActualVehicle = (Object *)
                   _methoda( ywd->world, YWM_CREATEVEHICLE, &cv ) ) )
                return( PARSE_BOGUS_DATA );

            /* ------------------------------------------------------
            ** Weiterhin gehört zur vorbereitenden Initialisierung
            ** das holen des Bacterien-Pointers und das Einklinken in
            ** eine Liste.
            ** ----------------------------------------------------*/
            _get( ActualVehicle, YBA_Bacterium, &ActualBacterium );            
            _methoda( ActualCommander, YBM_ADDSLAVE, ActualVehicle );

            return( PARSE_ENTERED_CONTEXT );
            }
        else {

            /*** Damit kann ich nichts anfangen ***/
            return( PARSE_UNKNOWN_KEYWORD );
            }
        }
    else {

        /*** Wir bearbeiten das schon. Ist es das Ende? ***/
        if( stricmp( parser->keyword, "end") == 0 ) {

            /*** Das Ende naht! ***/
            parser->status = PARSESTAT_READY;
            return( PARSE_LEFT_CONTEXT );
            }
        else {

            /*** Es sollte also ein  normales keyword sein ***/
            if( parser->target ) {

                /* -------------------------------------------------
                ** Weil viele daten gleich sind, gibt es dafür einen
                ** zentralen Auswerter...
                ** Dieser gibt TRUE zurück, wenn es für ihn bestimmt
                ** war.
                ** -----------------------------------------------*/
                
                if( !yw_ExtractVehicleValues( parser ) )
                    return( PARSE_UNKNOWN_KEYWORD );
                }

            return( PARSE_ALL_OK );
            }
        }
}


ULONG yw_ParseCommander( struct ScriptParser *parser )
{
    struct ypaworld_data *ywd;                                            

    if( parser->target ) ywd = parser->target;

    if( PARSESTAT_READY == parser->status ) {

        /*** Wir sind bereit für etwas neues ***/
        if( stricmp( parser->keyword, "begin_commander") == 0 ) {

            struct createvehicle_msg cv;

            /*** Das ist etwas für uns ***/
            parser->status = PARSESTAT_RUNNING;

            /* ----------------------------------------------------------
            ** in data steht jetzt die ID, wir erzeugen gleich ein Objekt
            ** dazu, welches wir später richtig positionieren
            ** --------------------------------------------------------*/
            cv.x    =  SECTOR_SIZE / 2;
            cv.z    = -SECTOR_SIZE / 2;
            cv.y    =  0.0;
            cv.vp   =  atoi( parser->data );
            if( !( ActualVehicle = (Object *)
                   _methoda( ywd->world, YWM_CREATEVEHICLE, &cv ) ) )
                return( PARSE_BOGUS_DATA );

            /* ------------------------------------------------------
            ** Weiterhin gehört zur vorbereitenden Initialisierung
            ** das holen des Bacterien-Pointers und das Einklinken in
            ** eine Liste.
            ** ----------------------------------------------------*/
            _get( ActualVehicle, YBA_Bacterium, &ActualBacterium );            
            ActualCommander = ActualVehicle;
            _methoda( ActualRobo, YBM_ADDSLAVE, ActualVehicle );

            return( PARSE_ENTERED_CONTEXT );
            }
        else {

            /*** Damit kann ich nichts anfangen ***/
            return( PARSE_UNKNOWN_KEYWORD );
            }
        }
    else {

        /*** Wir bearbeiten das schon. Ist es das Ende? ***/
        if( stricmp( parser->keyword, "end") == 0 ) {

            /*** Das Ende naht! ***/
            parser->status = PARSESTAT_READY;
            return( PARSE_LEFT_CONTEXT );
            }
        else {

            /*** Es sollte also ein  normales keyword sein ***/
            if( parser->target ) {

                /* -------------------------------------------------
                ** Weil viele daten gleich sind, gibt es dafür einen
                ** zentralen Auswerter...
                ** Dieser gibt TRUE zurück, wenn es für ihn bestimmt
                ** war.
                ** -----------------------------------------------*/
                
                if( !yw_ExtractVehicleValues( parser ) )
                    return( PARSE_UNKNOWN_KEYWORD );
                }

            return( PARSE_ALL_OK );
            }
        }
}


ULONG yw_ParseRobo( struct ScriptParser *parser )
{
    struct ypaworld_data *ywd = NULL;                                            

    if( parser->target ) ywd = parser->target;

    if( PARSESTAT_READY == parser->status ) {

        /*** Wir sind bereit für etwas neues ***/
        if( stricmp( parser->keyword, "begin_robo") == 0 ) {

            struct createvehicle_msg cv;

            /*** Das ist etwas für uns ***/
            parser->status = PARSESTAT_RUNNING;

            /* ----------------------------------------------------------
            ** in data steht jetzt die ID, wir erzeugen gleich ein Objekt
            ** dazu, welches wir später richtig positionieren
            ** --------------------------------------------------------*/
            cv.x    =  SECTOR_SIZE / 2;
            cv.z    = -SECTOR_SIZE / 2;
            cv.y    =  0.0;
            cv.vp   =  atoi( parser->data );
            if( !( ActualVehicle = (Object *)
                   _methoda( ywd->world, YWM_CREATEVEHICLE, &cv ) ) )
                return( PARSE_BOGUS_DATA );

            /* ------------------------------------------------------
            ** Weiterhin gehört zur vorbereitenden Initialisierung
            ** das holen des Bacterien-Pointers und das Einklinken in
            ** eine Liste.
            ** ----------------------------------------------------*/
            _get( ActualVehicle, YBA_Bacterium, &ActualBacterium );            
            ActualRobo = ActualVehicle;
            _methoda( ywd->world, YWM_ADDCOMMANDER, ActualVehicle );

            return( PARSE_ENTERED_CONTEXT );
            }
        else {

            /*** Damit kann ich nichts anfangen ***/
            return( PARSE_UNKNOWN_KEYWORD );
            }
        }
    else {

        /*** Wir bearbeiten das schon. Ist es das Ende? ***/
        if( stricmp( parser->keyword, "end") == 0 ) {

            /*** War es der Viewer-Robo? ***/
            if( 1 == ActualBacterium->Owner ) {
                ViewerRobo = ActualVehicle;

                /*** Der Welt melden ***/
                _set(ywd->world, YWA_UserRobo, ViewerRobo);
                //yrd->RoboState |= ROBO_USERROBO;
                }
                
            // FLOH: 22-Apr-98
            // falls <reload_const> nicht angegeben war, die Max-Energie nehmen 
            if (0 == ActualBacterium->RoboReloadConst) {
                ActualBacterium->RoboReloadConst = ActualBacterium->Maximum;
            };                
                
            /*** Das Ende naht! ***/
            parser->status = PARSESTAT_READY;
            return( PARSE_LEFT_CONTEXT );
            }
        else {

            /*** Es sollte also ein  normales keyword sein ***/
            if( parser->target ) {

                /* -------------------------------------------------
                ** Weil viele daten gleich sind, gibt es dafür einen
                ** zentralen Auswerter...
                ** Dieser gibt TRUE zurück, wenn es für ihn bestimmt
                ** war.
                ** -----------------------------------------------*/
                
                if( !yw_ExtractVehicleValues( parser ) )
                    if( !yw_ExtractRoboValues( parser ) )
                        return( PARSE_UNKNOWN_KEYWORD );
                }

            return( PARSE_ALL_OK );
            }
        }
}


BOOL yw_ExtractVehicleValues( struct ScriptParser *parser )
{
    /* -------------------------------------------------
    ** Jetzt kommt ein gewaltiges if-Konstrukt. Wenn wir
    ** das keyword nicht gefunden haben, geben wir FALSE
    ** zurück. Es gibt globale Variable, welches Objekt
    ** gerade bearbeitet wird. Diese nutzen wir für uns.
    ** Mit Ausnahme von Viewer etc. setze ich hier nur
    ** Werte, der rest passiert in Reorganise, damit ich
    ** zum Bsp. garantieren kann, daß MainState vor
    ** ExtraState gesetzt wird und anderes!
    ** -----------------------------------------------*/

    /*** Allgemeines ***/
    if( stricmp( parser->keyword, "viewer" ) == 0 ) {

        /*** ist er Viewer? ***/
        if( stricmp( parser->data, "yes") == 0 )
            _set( ActualVehicle, YBA_Viewer, TRUE );
        }
    else {

    if( stricmp( parser->keyword, "user" ) == 0 ) {

        /*** ist er HI-berechtigt? ***/
        if( stricmp( parser->data, "yes") == 0 )
            _set( ActualVehicle, YBA_UserInput, TRUE );
        }
    else {

    if( stricmp( parser->keyword, "collision" ) == 0 ) {

        /*** ist er HI-berechtigt? ***/
        if( stricmp( parser->data, "yes") == 0 )
            _set( ActualVehicle, YBA_BactCollision, TRUE );
        }
    else {

    if( stricmp( parser->keyword, "commandid" ) == 0 ) {

        /*** eintragen und in Roboclass aktualisieren ***/
        ActualBacterium->CommandID = atol( parser->data );
        if( CommandCount < ActualBacterium->CommandID )
            CommandCount = ActualBacterium->CommandID;
        }
    else {

    if( stricmp( parser->keyword, "aggression" ) == 0 ) {

        /*** nur eintragen ***/
        ActualBacterium->Aggression = (UBYTE) atoi( parser->data );
        }
    else {

    if( stricmp( parser->keyword, "mainstate" ) == 0 ) {

        /*** eintragen und setzen ***/
        ActualBacterium->MainState = atoi( parser->data );
        }
    else {

    if( stricmp( parser->keyword, "extrastate" ) == 0 ) {

        /*** nur eintragen ***/
        ActualBacterium->ExtraState = atol( parser->data );
        }
    else {

    if( stricmp( parser->keyword, "killerowner" ) == 0 ) {

        /*** nur eintragen ***/
        ActualBacterium->killer_owner = atol( parser->data );
        }
    else {

    if( stricmp( parser->keyword, "ident" ) == 0 ) {

        /*** eintragen und DeathCount aktualisieren ***/
        ActualBacterium->ident = atol( parser->data );
        if( DeathCount < ActualBacterium->ident )
            DeathCount = ActualBacterium->ident;
        }
    else {

    if( stricmp( parser->keyword, "primary" ) == 0 ) {

        /* ---------------------------------------------------
        ** Wir tragen die Werte nur ein, setzen aber nichts.
        ** Anstelle des Bactpointers merken wir uns ident und
        ** erst reorganise macht einen Bakterienpointer draus,
        ** sofern notwendig, weil es sein kann, daß das Ziel
        ** noch nicht existiert.
        ** -------------------------------------------------*/

        char *ptr;

        if( ptr = strtok( parser->data, " \t_\n" ) ) {

            /*** Typ ***/
            ActualBacterium->PrimTargetType = (UBYTE) atoi( ptr );
            if( ptr = strtok( NULL, " \t_\n" ) ) {

                /*** ident des Bacteriums ***/
                ActualBacterium->PrimaryTarget.Bact = (struct Bacterium *) atol( ptr );
                if( ptr = strtok( NULL, " \t_\n" ) ) {

                    /*** Pos x ***/
                    ActualBacterium->PrimPos.x = atof( ptr );
                    if( ptr = strtok( NULL, " \t_\n" ) ) {

                        /*** Typ ***/
                        ActualBacterium->PrimPos.z = atof( ptr );
                        if( ptr = strtok( NULL, " \t_\n" ) ) {

                            /*** PrimCommandID ***/
                            ActualBacterium->PrimCommandID = atol( ptr );
                            }
                        }
                    }
                }
            }
        }
    else {

    /*** Physikalische Eigenschaften ***/
    if( stricmp( parser->keyword, "speed" ) == 0 ) {

        char *ptr;

        if( ptr = strtok( parser->data, " \t_\n" ) ) {

            /*** x ***/
            ActualBacterium->dof.x = atof( parser->data );
            if( ptr = strtok( NULL, " \t_\n" ) ) {

                /*** y ***/
                ActualBacterium->dof.y = atof( parser->data );
                if( ptr = strtok( NULL, " \t_\n" ) ) {

                    /*** z ***/
                    ActualBacterium->dof.z = atof( parser->data );
                    if( ptr = strtok( NULL, " \t_\n" ) ) {

                        /*** v ***/
                        ActualBacterium->dof.v = atof( parser->data );
                        }
                    }
                }
            }
        }
    else {

    if( stricmp( parser->keyword, "energy" ) == 0 ) {

        ActualBacterium->Energy  = atol( parser->data );
        }
    else {

    if( stricmp( parser->keyword, "matrix" ) == 0 ) {

        char *ptr;

        if( ptr = strtok( parser->data, " \t_\n") ) {

          ActualBacterium->dir.m11 = atof( ptr );
          if( ptr = strtok( NULL, " \t_\n") ) {

            ActualBacterium->dir.m12 = atof( ptr );
            if( ptr = strtok( NULL, " \t_\n") ) {

              ActualBacterium->dir.m13 = atof( ptr );
              if( ptr = strtok( NULL, " \t_\n") ) {

                ActualBacterium->dir.m21 = atof( ptr );
                if( ptr = strtok( NULL, " \t_\n") ) {

                  ActualBacterium->dir.m22 = atof( ptr );
                  if( ptr = strtok( NULL, " \t_\n") ) {

                    ActualBacterium->dir.m23 = atof( ptr );
                    if( ptr = strtok( NULL, " \t_\n") ) {

                      ActualBacterium->dir.m31 = atof( ptr );
                      if( ptr = strtok( NULL, " \t_\n") ) {

                        ActualBacterium->dir.m32 = atof( ptr );
                        if( ptr = strtok( NULL, " \t_\n") ) {

                          ActualBacterium->dir.m33 = atof( ptr );
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
    else {

    if( stricmp( parser->keyword, "pos" ) == 0 ) {

        /*** Wie immer: Nur ausfüllen ***/
        char *ptr;

        if( ptr = strtok( parser->data, " \t_\n") ) {

            /*** x ***/
            ActualBacterium->pos.x = atof( ptr );
            if( ptr = strtok( NULL, " \t_\n") ) {

                /*** y ***/
                ActualBacterium->pos.y = atof( ptr );
                if( ptr = strtok( NULL, " \t_\n") ) {

                    /*** z ***/
                    ActualBacterium->pos.z = atof( ptr );
                    }
                }
            }
        }
    else {

    if( stricmp( parser->keyword, "force" ) == 0 ) {

        ActualBacterium->act_force = atof( parser->data );
        }
    else {

    if( stricmp( parser->keyword, "gunangle" ) == 0 ) {

        ActualBacterium->gun_angle_user = atof( parser->data );
        }
    else {

    if( stricmp( parser->keyword, "gunbasis" ) == 0 ) {

        struct ypagun_data *ygd;
        char   *ptr;

        ygd = INST_DATA( ((struct nucleusdata *)ActualVehicle)->o_Class, ActualVehicle);

        if( ptr = strtok( parser->data, " \t_\n") ) {

            /*** x ***/
            ygd->basis.x = atof( ptr );
            if( ptr = strtok( NULL, " \t_\n") ) {

                /*** y ***/
                ygd->basis.y = atof( ptr );
                if( ptr = strtok( NULL, " \t_\n") ) {

                    /*** z ***/
                    ygd->basis.z = atof( ptr );
                    }
                }
            }
        }
    else {

    if( stricmp( parser->keyword, "waypoint" ) == 0 ) {

        /*** Offset_x_y, wobei position FLOAT ist ***/
        char *ptr;
        int  offset;

        if( ptr = strtok( parser->data, " \t_\n") ) {

            offset = atoi( ptr );
            if( ptr = strtok( NULL, " \t_\n") ) {

                ActualBacterium->waypoint[ offset ].x = atof( ptr );
                if( ptr = strtok( NULL, " \t_\n") ) {

                    ActualBacterium->waypoint[ offset ].z = atof( ptr );
                    }
                }
            }
        }
    else {

    if( stricmp( parser->keyword, "num_wp" ) == 0 ) {

        ActualBacterium->num_waypoints = (WORD) atoi( parser->data );
        }
    else {

    if( stricmp( parser->keyword, "count_wp" ) == 0 ) {

        ActualBacterium->count_waypoints = (WORD) atoi( parser->data );
        }
    else {

        /*** Schade, nicht gefunden ***/
        return( FALSE );
        } } } } } } } } } } } } } } } } } } } }

    return( TRUE );
}


BOOL yw_ExtractRoboValues( struct ScriptParser *parser )
{
    /* ------------------------------------------------
    ** Nun kommen die speziellen Robosachen. naja, Zeug
    ** eben.
    ** ----------------------------------------------*/
    struct ypaworld_data *ywd;
    struct yparobo_data *yrd;
    yrd = INST_DATA( ((struct nucleusdata *)ActualVehicle)->o_Class,
                     ActualVehicle );
    ywd = INST_DATA( ((struct nucleusdata *)(yrd->world))->o_Class,
                     yrd->world );

    if( stricmp( parser->keyword, "owner" ) == 0 ) {
        ActualBacterium->Owner = (UBYTE) atoi( parser->data );
    } else if( stricmp( parser->keyword, "robostate" ) == 0 ) {
        yrd->RoboState = atol(parser->data);
    } else if( stricmp( parser->keyword, "dockenergy" ) == 0 ) {
        yrd->dock_energy = atol( parser->data );
    } else if( stricmp( parser->keyword, "dockcount" ) == 0 ) {
        yrd->dock_count = atol( parser->data );
    } else if( stricmp( parser->keyword, "dockuser" ) == 0 ) {
        yrd->dock_user = atol( parser->data );
    } else if( stricmp( parser->keyword, "docktime" ) == 0 ) {
        yrd->dock_time = atol( parser->data );
    } else if( stricmp( parser->keyword, "docktargettype" ) == 0 ) {
        yrd->dttype = (UBYTE)atol( parser->data );
    } else if( stricmp( parser->keyword, "dockaggr" ) == 0 ) {
        yrd->dock_aggr = (UBYTE)atol( parser->data );
    } else if( stricmp( parser->keyword, "docktargetpos" ) == 0 ) {

        char *ptr;

        /*** Die x- und die z-Koordinate stehen hier drinnen ***/
        if( ptr = strtok( parser->data, " \t_\n") ) {
            yrd->dtpos.x = atof( ptr );
            if( ptr = strtok( NULL, " \t_\n") ) {
                yrd->dtpos.z = atof( ptr );
            }
        }
    } else if( stricmp( parser->keyword, "docktargetID" ) == 0 ) {
        yrd->dtCommandID = atol( parser->data );
    } else if( stricmp( parser->keyword, "fillmodus" ) == 0 ) {
        yrd->FillModus = (UBYTE) atoi( parser->data );
    } else if( stricmp( parser->keyword, "battbuilding" ) == 0 ) {
        // gibts nicht mehr
    } else if( stricmp( parser->keyword, "battvehicle" ) == 0 ) {
        yrd->BattVehicle = atol( parser->data );
    } else if( stricmp( parser->keyword, "buildspare" ) == 0 ) {
        yrd->BuildSpare = atol( parser->data );
    } else if( stricmp( parser->keyword, "battbeam" ) == 0 ) {
        yrd->BattBeam = atol( parser->data );
    } else if( stricmp( parser->keyword, "vhoriz" ) == 0 ) {
        yrd->bact->Viewer.horiz_angle = atof( parser->data );
    } else if( stricmp( parser->keyword, "vvert" ) == 0 ) {
        yrd->bact->Viewer.vert_angle = atof( parser->data );
    } else if( stricmp( parser->keyword, "maximum" ) == 0 ) {

        /*** extra, weil nicht an Visproto gekoppelt! ***/
        yrd->bact->Maximum = atol( parser->data );

        /*** Maximum unter bisheriger Höchstgrenze ***/
        if( 1 == yrd->bact->Owner ) {   // Owner noch verallgmeinern!
            if( yrd->bact->Maximum < ywd->MaxRoboEnergy ) {
                yrd->bact->Maximum         = ywd->MaxRoboEnergy;
                yrd->bact->RoboReloadConst = ywd->MaxReloadConst;
            };
        }
    } else if( stricmp( parser->keyword, "con_budget" ) == 0 ) {
        yrd->ep_Conquer = (UBYTE) atoi( parser->data );
    } else if( stricmp( parser->keyword, "def_budget" ) == 0 ) {
        yrd->ep_Defense = (UBYTE) atoi( parser->data );
    } else if( stricmp( parser->keyword, "rec_budget" ) == 0 ) {
        yrd->ep_Reconnoitre = (UBYTE) atoi( parser->data );
    } else if( stricmp( parser->keyword, "rob_budget" ) == 0 ) {
        yrd->ep_Robo = (UBYTE) atoi( parser->data );
    } else if( stricmp( parser->keyword, "rad_budget" ) == 0 ) {
        yrd->ep_Radar = (UBYTE) atoi( parser->data );
    } else if( stricmp( parser->keyword, "pow_budget" ) == 0 ) {
        yrd->ep_Power = (UBYTE) atoi( parser->data );
    } else if( stricmp( parser->keyword, "saf_budget" ) == 0 ) {
        yrd->ep_Safety = (UBYTE) atoi( parser->data );
    } else if( stricmp( parser->keyword, "cpl_budget" ) == 0 ) {
        yrd->ep_ChangePlace = (UBYTE) atoi( parser->data );
    } else if( stricmp( parser->keyword, "saf_delay" ) == 0 ) {
        yrd->chk_Safety_Delay = (LONG) atol( parser->data );
    } else if( stricmp( parser->keyword, "pow_delay" ) == 0 ) {
        yrd->chk_Power_Delay = (LONG) atol( parser->data );
    } else if( stricmp( parser->keyword, "rad_delay" ) == 0 ) {
        yrd->chk_Radar_Delay = (LONG) atol( parser->data );
    } else if( stricmp( parser->keyword, "cpl_delay" ) == 0 ) {
        yrd->chk_Place_Delay = (LONG) atol( parser->data );
    } else if( stricmp( parser->keyword, "def_delay" ) == 0 ) {
        yrd->chk_Enemy_Delay = (LONG) atol( parser->data );
    } else if( stricmp( parser->keyword, "con_delay" ) == 0 ) {
        yrd->chk_Terr_Delay = (LONG) atol( parser->data );
    } else if( stricmp( parser->keyword, "rec_delay" ) == 0 ) {
        yrd->chk_Recon_Delay = (LONG) atol( parser->data );
    } else if( stricmp( parser->keyword, "rob_delay" ) == 0 ) {
        yrd->chk_Robo_Delay = (LONG) atol( parser->data );
        
    } else if( stricmp( parser->keyword, "reload_const" ) == 0 ) {
        // FLOH: 22-Apr-98
        /*** ReloadConst nur ueberschreiben, falls nicht schon durch MaxEnergy passiert ***/        
        if (yrd->bact->RoboReloadConst == 0) {
            yrd->bact->RoboReloadConst = (LONG) atol(parser->data);
        };
    } else {
        /*** nicht gefunden ***/
        return ( FALSE );
    }
    return( TRUE );
}


void yw_ReorganizeVehicles( struct ypaworld_data *ywd )
{
    /* -----------------------------------------------------
    ** Nun haben wir alle Vehicle in eine Struktur gebracht.
    ** jetzt gehen wir alles noch einmal durch und machen
    ** so diverse Sachen mit denen
    ** ---------------------------------------------------*/

    struct OBNode *robo, *user_robo;

    robo = (struct OBNode *) ywd->CmdList.mlh_Head;
    while( robo->nd.mln_Succ ) {

        struct OBNode *commander;

        /*** Die Sachen..., bei Robos wird ein TRUE übergeben ***/
        yw_RefreshVehicle( ywd, robo, robo, TRUE );

        /*** Userrobo merken für später ***/
        if( 1 == robo->bact->Owner ) user_robo = robo;

        commander = (struct OBNode *) robo->bact->slave_list.mlh_Head;

        while( commander->nd.mln_Succ ) {

            struct OBNode *slave;

            /*** Die Sachen..., bei Commandern wird ein FALSE übergeben ***/
            yw_RefreshVehicle( ywd, commander, robo, FALSE );

            slave = (struct OBNode *) commander->bact->slave_list.mlh_Head;

            while( slave->nd.mln_Succ ) {

                /*** Die Sachen..., bei Slaves wird FALSE übergeben ***/
                yw_RefreshVehicle( ywd, slave, robo, FALSE );

                slave = (struct OBNode *) slave->nd.mln_Succ;
                }

            commander = (struct OBNode *) commander->nd.mln_Succ;
            }

        robo = (struct OBNode *) robo->nd.mln_Succ;
        }

    /*** nachträgliches Viewer-Setzen ***/
    if( EXTRA_VIEWER == EV_RoboGun ) {

        struct yparobo_data *yrd;

        yrd = INST_DATA( ((struct nucleusdata *)(user_robo->o))->o_Class, user_robo->o);
        if( yrd->gun[ VIEWER_NUMBER ].go ) {

            _set( yrd->gun[ VIEWER_NUMBER ].go, YBA_Viewer,    TRUE );
            _set( yrd->gun[ VIEWER_NUMBER ].go, YBA_UserInput, TRUE );
            }
        }
}


void yw_RefreshVehicle( struct ypaworld_data *ywd,
                        struct OBNode *vehicle,
                        struct OBNode *robo,
                        BOOL   is_robo )
{
    // pos, state, extra, target, owner, robopointer
    struct setposition_msg sp;
    struct setstate_msg state;

    /*** Nun wird es konkret ***/

    /*** Setzen der Position ***/
    sp.x     = vehicle->bact->pos.x;
    sp.y     = vehicle->bact->pos.y;
    sp.z     = vehicle->bact->pos.z;
    if( BCLID_YPAGUN == vehicle->bact->BactClassID )
        sp.flags = YGFSP_SetFree;
    else
        sp.flags = 0;
    _methoda( vehicle->o, YBM_SETPOSITION, &sp );

    /*** Kanone installieren ***/
    if( BCLID_YPAGUN == vehicle->bact->BactClassID ) {

        struct ypagun_data *ygd;
        struct installgun_msg ig;

        ygd = INST_DATA( ((struct nucleusdata *)(vehicle->o))->o_Class, vehicle->o);

        ig.basis = ygd->basis;
        ig.flags = 0;
        _methoda( vehicle->o, YGM_INSTALLGUN, &ig );
        }

    /* ---------------------------------------------------------------
    ** Wenn es Soundprobleme gibt, dann Protos löschen, weil der Sound
    ** nicht gesetzt wird, wenn der Zustand sich nicht ändert
    ** -------------------------------------------------------------*/

    /*** Mainstate setzen ***/
    state.main_state = vehicle->bact->MainState;
    state.extra_off  = state.extra_on = 0L;
    _methoda( vehicle->o, YBM_SETSTATE, &state );

    /*** ExtraState abhandeln ***/
    if( vehicle->bact->ExtraState & EXTRA_MEGADETH ) {

        /*** megatöteln ***/
        state.main_state = 0;
        state.extra_off  = 0L;
        state.extra_on   = EXTRA_MEGADETH;
        _methoda( vehicle->o, YBM_SETSTATE, &state );
        }
    if( vehicle->bact->ExtraState & EXTRA_FIRE ) {

        /*** feuerzündeln ***/
        state.main_state = 0;
        state.extra_off  = 0L;
        state.extra_on   = EXTRA_FIRE;
        _methoda( vehicle->o, YBM_SETSTATE, &state );
        }


    if( is_robo ) {

        /*** evtl. als UserRobo anmelden ***/
        if( 1 == vehicle->bact->Owner ) {

            _set(ywd->world, YWA_UserRobo, vehicle->o);
            /*** roboState-Flag dürfte schon gesetzt sein ***/
            }
        }
    else {

        /*** Robopointer ausfüllen ***/
        vehicle->bact->robo = robo->o;

        /*** Owner setzen ***/
        vehicle->bact->Owner = robo->bact->Owner;
        }

    /*** PrimaryTarget ***/
    if( TARTYPE_BACTERIUM == vehicle->bact->PrimTargetType ) {

        struct Bacterium *b;
        struct settarget_msg target;

        /*** Pointer holen und Ziel neu Setzen ***/
        b = yw_GetBactFromIdent((ULONG)(vehicle->bact->PrimaryTarget.Bact),ywd);
        vehicle->bact->PrimTargetType = TARTYPE_NONE;   // Vorbereitung!
        target.target_type = TARTYPE_BACTERIUM_NR;
        target.target.bact = b;
        target.priority    = 0;
        _methoda( vehicle->o, YBM_SETTARGET, &target );
        }

    if( TARTYPE_SECTOR == vehicle->bact->PrimTargetType ) {

        struct settarget_msg target;

        /*** Pointer holen und Ziel neu Setzen ***/
        vehicle->bact->PrimTargetType = TARTYPE_NONE;   // Vorbereitung!
        target.target_type = TARTYPE_SECTOR_NR;
        target.pos         = vehicle->bact->PrimPos;
        target.priority    = 0;
        _methoda( vehicle->o, YBM_SETTARGET, &target );
        }
}


struct Bacterium *yw_GetBactFromIdent( ULONG ident, struct ypaworld_data *ywd )
{
    /* -----------------------------------------------------
    ** Liefert zu der ident-Nummer eine Bact-Struktur aus
    ** der bestehenden Geschwaderstruktur. Die Netzwerk-
    ** spezifischen Routinen in network.c arbeiten schneller
    ** und mit Spezialinfos. Deshalb 2 Routinen...
    ** ---------------------------------------------------*/

    struct OBNode *robo;

    robo = (struct OBNode *) ywd->CmdList.mlh_Head;
    while( robo->nd.mln_Succ ) {

        struct OBNode *commander;

        /*** gefunden ? ***/
        if( robo->bact->ident == ident ) return( robo->bact );

        commander = (struct OBNode *) robo->bact->slave_list.mlh_Head;

        while( commander->nd.mln_Succ ) {

            struct OBNode *slave;

            /*** gefunden ? ***/
            if( commander->bact->ident == ident ) return( commander->bact );

            slave = (struct OBNode *) commander->bact->slave_list.mlh_Head;

            while( slave->nd.mln_Succ ) {

                /*** gefunden ? ***/
                if( slave->bact->ident == ident ) return( slave->bact );

                slave = (struct OBNode *) slave->nd.mln_Succ;
                }

            commander = (struct OBNode *) commander->nd.mln_Succ;
            }

        robo = (struct OBNode *) robo->nd.mln_Succ;
        }

    /*** nix gefunden ***/
    return( NULL );
}

BOOL yw_RestoreVehicleData( struct ypaworld_data *ywd )
{
    /* ------------------------------------------------------------
    ** Lädt aus dem Restartfile alle VehicleInformationen.
    ** Dies dient dazu, bei Abbrüchen den originalen Zustand wieder
    ** herzustellen.
    ** ----------------------------------------------------------*/
    struct ScriptParser parser[ 3 ];
    char   filename[ 255 ];

    if( ywd->gsr ) {

        memset(&parser, 0, sizeof(parser));
        sprintf( filename, "save:%s/%d.rst\0", ywd->gsr->UserName,
                                               ywd->Level->Num );

        parser[0].parse_func = yw_VhclProtoParser;  // für bauflags, energy, shield...
        parser[0].store[0]   = (ULONG) ywd;

        parser[1].parse_func = yw_WeaponProtoParser;
        parser[1].store[0]   = (ULONG) ywd;

        parser[2].parse_func = yw_BuildProtoParser;
        parser[2].store[1]   = (ULONG) ywd;

        /*** nu los ***/
        return( yw_ParseScript( filename, 3, parser, PARSEMODE_SLOPPY ) );
        }
    else return( TRUE );
}


BOOL yw_ExistSaveGame( ULONG slot, char *User )
{
    /* ------------------------------------------------------
    ** Testet, ob es das Savegame gibt. Ist dem so, geben wir
    ** TRUE zurück. Wir testen einfach das File.
    ** NEU: Der Filename besteht aus "Username/%d.sgm" mit
    ** dem übergebenen Slot.
    ** Leerzeichen müssen rausgefiltert werden!
    ** ----------------------------------------------------*/

    FILE  *TF;
    UBYTE  testname[ 300 ];
    sprintf( testname, "save:%s/%d.sgm\0", User, slot );
    if( !(TF = _FOpen( testname, "r"))) return( FALSE ); else _FClose( TF );
    return( TRUE );
}


BOOL yw_ExistFinalSaveGame( ULONG level, char *User )
{
    /* ------------------------------------------------------
    ** Testet, ob es das Savegame gibt. Ist dem so, geben wir
    ** TRUE zurück. Wir testen einfach das File.
    ** NEU: Der Filename besteht aus "Username/%d.sgm" mit
    ** dem übergebenen Level.
    ** Leerzeichen müssen rausgefiltert werden!
    ** ----------------------------------------------------*/

    FILE  *TF;
    UBYTE  testname[ 300 ];
    sprintf( testname, "save:%s/%d.fin\0", User, level );
    if( !(TF = _FOpen( testname, "r"))) return( FALSE ); else _FClose( TF );
    return( TRUE );
}


BOOL yw_ExistRestartSaveGame( ULONG level, char *User )
{
    /* ------------------------------------------------------
    ** Testet, ob es das Savegame gibt. Ist dem so, geben wir
    ** TRUE zurück. Wir testen einfach das File.
    ** NEU: Der Filename besteht aus "Username/%d.sgm" mit
    ** dem übergebenen Slot.
    ** Leerzeichen müssen rausgefiltert werden!
    ** ----------------------------------------------------*/
    FILE  *TF;
    UBYTE testname[ 300 ];
    sprintf( testname, "save:%s/%d.rst\0", User, level );
    if( !(TF = _FOpen( testname, "r"))) return( FALSE ); else _FClose( TF );
    return( TRUE );
}

BOOL yw_ExistFileName( char *name )
{
    /* ------------------------------------------------------
    ** Testet nur, ob es das File des angegebenen Namens gibt
    ** Ersetzt auch Leerzeichen
    ** ----------------------------------------------------*/
    FILE  *TF;
    UBYTE testname[ 300 ];

    sprintf( testname, "%s\0", name );

    if( !(TF = _FOpen( testname, "r"))) return( FALSE ); else _FClose( TF );

    return( TRUE );
}

