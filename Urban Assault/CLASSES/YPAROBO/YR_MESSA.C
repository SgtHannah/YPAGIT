/*
**  $Source: $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:24:47 $
**  $Locker: $
**  $Author: $
**
**  Die Messageroutinen
**
**  (C) Copyright 1995 by Andreas Flemming
*/
#include <exec/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
_extern_use_audio_engine

BOOL yr_IsMessageNew( struct yparobo_data *yrd, struct bact_message *message,
                      UBYTE **lhandle );
char *yr_GetMessageString( UBYTE **lhandle, LONG ID );

/* ---------------------------------------------------------------------
** wir bauen das globale Array einfach aus log-messages auf, da brauchen
** wir nix neues zu deklarieren
** -------------------------------------------------------------------*/
struct bact_message MessageArray[ NUM_LOGMESSAGES ];
LONG   robotime[ 8 ] = {0, 0, 0, 0, 0, 0, 0, 0};


_dispatcher( BOOL, yr_YRM_LOGMSG, struct bact_message *message )
{
/*
**  FUNCTION    verwaltet die messages und läßt nur das wichtigste
**              an die Welt durch. Schön wäre es auch, wenn ich hier
**              den Sound mit abhandeln würde, weil hier eh bloß
**              Nummern reinkommen.
**
**              Ich merke mir von allen möglichen Messages die zuletzt
**              reingekommenen und akzeptiere neue dieser Art erst
**              wieder, wenn sie sich wesentlich unterscheiden
**
**  INPUT       Die Message mit ihren Parametern
**
**  OUTPUT      TRUE, wenn die Message übernommen und damit an die Welt
**              weitergeleitet wurde.
**
**  CHANGED     Uli zahlt immer noch nicht!
*/

    struct yparobo_data *yrd = INST_DATA( cl, o );
    char   *what;
    UBYTE  **lhandle;
    struct logmsg_msg log;
    
    /* -------------------------------------------------------------
    ** Guns geben keine Meldungen ab, vielleicht später nur RoboGuns
    ** dahingehend betrachten
    ** -----------------------------------------------------------*/
    if( (message->sender) &&
        (BCLID_YPAGUN == message->sender->BactClassID) ) {

        /* --------------------------------------------------------
        ** manche Messages dürfen von Flaks (Robo oder nicht) nicht
        ** abgegeben werden. UNVOLLSTÄNDIG!!!!!
        ** ------------------------------------------------------*/
        if( (message->ID != LOGMSG_FOUNDENEMY) &&
            (message->ID != LOGMSG_ENEMYESCAPES) &&
            (message->ID != LOGMSG_FOUNDROBO) ) {

            return( FALSE );
            }
        }

    /*** Locale-Handle holen ***/
    _get( yrd->world, YWA_LocaleHandle, &lhandle );

    if( yr_IsMessageNew( yrd, message, lhandle ) ) {

        /*** Wer und Woher sind raus ***/

        /*** String kann NULL sein ***/
        what = yr_GetMessageString( lhandle, message->ID );

        /* ---------------------------------------------------------
        ** Abschicken der Message. Neu ist das Codefeld, in welchem
        ** ich eine ID übergebe. Das ist praktischerweise die ID der
        ** übergebenen Message. 
        ** -------------------------------------------------------*/
        log.pri  = message->pri;
        log.msg  = what;
        log.bact = message->sender;
        log.code = message->ID;
        _methoda( yrd->world, YWM_LOGMSG, &log );

        return( TRUE );
        }

    return( FALSE );
}


char *yr_GetMessageString( UBYTE **lhandle, LONG ID )
{
    /* ----------------------------------------------------------
    ** Sucht zur ID den zugehörigen String und gibt ihn localized
    ** zurück
    ** --------------------------------------------------------*/

    char *str = NULL;

    switch( ID ) {

        case LOGMSG_DONE:

            //str = ypa_GetStr( lhandle, STR_LMSG_DONE, DEF_LMSG_DONE );
            break;

        case LOGMSG_CONQUERED:

            str = ypa_GetStr( lhandle, STR_LMSG_CONQUERED, DEF_LMSG_CONQUERED );
            break;

        case LOGMSG_FIGHTSECTOR:

            //str = ypa_GetStr( lhandle, STR_LMSG_FIGHTSECTOR, DEF_LMSG_FIGHTSECTOR );
            break;

        case LOGMSG_REMSECTOR:

            //str = ypa_GetStr( lhandle, STR_LMSG_REMSECTOR, DEF_LMSG_REMSECTOR );
            break;

        case LOGMSG_ENEMYDESTROYED:

            //str = ypa_GetStr( lhandle, STR_LMSG_ENEMYDESTROYED, DEF_LMSG_ENEMYDESTROYED );
            break;

        case LOGMSG_FOUNDROBO:

            str = ypa_GetStr( lhandle, STR_LMSG_FOUNDROBO, DEF_LMSG_FOUNDROBO );
            break;

        case LOGMSG_FOUNDENEMY:

            //str = ypa_GetStr( lhandle, STR_LMSG_FOUNDENEMY, DEF_LMSG_FOUNDENEMY );
            break;

        case LOGMSG_LASTDIED:

            //str = ypa_GetStr( lhandle, STR_LMSG_LASTDIED, DEF_LMSG_LASTDIED );
            break;

        case LOGMSG_ESCAPE:

            //str = ypa_GetStr( lhandle, STR_LMSG_ESCAPE, DEF_LMSG_ESCAPE );
            break;

        case LOGMSG_RELAXED:

            //str = ypa_GetStr( lhandle, STR_LMSG_RELAXED, DEF_LMSG_RELAXED );
            break;

        case LOGMSG_ROBODEAD:

            str = ypa_GetStr( lhandle, STR_LMSG_ROBODEAD, DEF_LMSG_ROBODEAD );
            break;

        case LOGMSG_USERROBODEAD:

            str = ypa_GetStr( lhandle, STR_LMSG_USERROBODEAD, DEF_LMSG_USERROBODEAD );
            break;

        case LOGMSG_USERROBODANGER:

            str = ypa_GetStr( lhandle, STR_LMSG_USERROBODANGER, DEF_LMSG_USERROBODANGER );
            break;

        case LOGMSG_REQUESTSUPPORT:

            str = ypa_GetStr( lhandle, STR_LMSG_REQUESTSUPPORT, DEF_LMSG_REQUESTSUPPORT );
            break;

        case LOGMSG_ENEMYESCAPES:

            //str = ypa_GetStr( lhandle, STR_LMSG_ENEMYESCAPES, DEF_LMSG_ENEMYESCAPES );
            break;

        case LOGMSG_FLAK_DESTROYED:

            str = ypa_GetStr( lhandle, STR_LMSG_FLAK_DESTROYED, DEF_LMSG_FLAK_DESTROYED );
            break;

        case LOGMSG_RADAR_DESTROYED:

            str = ypa_GetStr( lhandle, STR_LMSG_RADAR_DESTROYED, DEF_LMSG_RADAR_DESTROYED );
            break;

        case LOGMSG_HOST_ENERGY_CRITICAL:

            str = ypa_GetStr( lhandle, STR_LMSG_HOST_ENERGY_CRITICAL, DEF_LMSG_HOST_ENERGY_CRITICAL );
            break;

        case LOGMSG_POWER_ATTACK:

            str = ypa_GetStr( lhandle, STR_LMSG_POWER_ATTACK, DEF_LMSG_POWER_ATTACK );
            break;

        case LOGMSG_FLAK_ATTACK:

            str = ypa_GetStr( lhandle, STR_LMSG_FLAK_ATTACK, DEF_LMSG_FLAK_ATTACK );
            break;

        case LOGMSG_RADAR_ATTACK:

            str = ypa_GetStr( lhandle, STR_LMSG_RADAR_ATTACK, DEF_LMSG_RADAR_ATTACK );
            break;

        default:

            str = NULL;
            break;
        }

    return( str );
}


BOOL yr_IsMessageNew( struct yparobo_data *yrd, struct bact_message *message,
                      UBYTE **lhandle )
{
    /* ----------------------------------------------------------
    ** Ermittelt an Hand von Parametern, ob Msg wirklich neu ist.
    ** Wenn ja, dann werden die neuen Einstellungen gemerkt und
    ** TRUE zurückgegeben.
    ** --------------------------------------------------------*/

    BOOL   ret;
    Object *uv;
    struct Bacterium *bact;
    LONG   para1;

    /*** Debug-Filter ***/
    //if( message->ID != LOGMSG_HOST_ENERGY_CRITICAL) return( FALSE );

    switch( message->ID ) {

        /* -----------------------------------------------------------
        ** Wenn seit einiger Zeit keine Meldung reinkam, dann war
        ** er mal kurz unsichtbar, denn sonst hätte es einen Meldungs-
        ** versuch geben. Wenn sich Parameter1 (Owner) geändert hat
        ** oder wenn lange nichts kam --> Ausgeben. Das kann bei mehreren
        ** sichtbaren Robos Probleme bringen, weil sich dann der Owner
        ** permanent ändert. Deshalb Zeit Extra merken. Bei den
        ** Commandern umgehen wir das problem mit einem "&&".
        ** ---------------------------------------------------------*/
        case LOGMSG_FOUNDROBO:

            para1 = message->para1;

            if( (yrd->bact->internal_time - robotime[ para1 ] > 45000) ||
                (robotime[ para1 ] == 0) )
                ret = TRUE;
            else
                ret = FALSE;

            /*** Weil ja eine Meldung versucht wurde ***/
            robotime[ para1 ] = yrd->bact->internal_time;

            break;
        /* ---------------------------------------------------------
        ** Bei folgenden Sachen entscheidet der erste Parameter, der
        ** die eigene oder eine fremde GeschwaderID sein kann
        ** ESCAPE:        eigene ID
        ** RELAXED:       eigene ID
        ** FOUNDENEMY:    fremde ID
        ** USERROBODANGER fremde ID
        ** -------------------------------------------------------*/

        case LOGMSG_ESCAPE:
        case LOGMSG_RELAXED:
        case LOGMSG_USERROBODANGER:

            if( MessageArray[ message->ID ].para1 != message->para1 )
                ret = TRUE;
            else
                ret = FALSE;

            break;

        /* --------------------------------------------------------------------
        ** Diese meldungen müssen nur zeitlich ausgebremst werden.
        ** Weil die zeiten verschieden sind, werden sie als Parameter übergeben
        ** ------------------------------------------------------------------*/
        case LOGMSG_REQUESTSUPPORT:
        case LOGMSG_HOST_ENERGY_CRITICAL:

            if( yrd->bact->internal_time - MessageArray[ message->ID ].time >
                 message->para1 ) {

                MessageArray[ message->ID ].time = yrd->bact->internal_time;
                ret = TRUE;
                }
            else
                ret = FALSE;
            break;

        /* ----------------------------------------------------------
        ** Hier entscheiden 2 Sachen, also para1 und para2, was meist
        ** Sektorkoordinaten sind, von denen sich eine geändert haben
        ** kann. bei FoundRobo ist es der eigentümer des Robos und
        ** die CommandID des Senders/Entdeckers
        ** --------------------------------------------------------*/

        case LOGMSG_CONQUERED:
        case LOGMSG_FIGHTSECTOR:
        case LOGMSG_REMSECTOR:

            if( (MessageArray[ message->ID ].para1 != message->para1) ||
                (MessageArray[ message->ID ].para2 != message->para2) )
                ret = TRUE;
            else
                ret = FALSE;
            break;

        /* -------------------------------------------------------------
        ** Hier entscheidet gar nix, will heißen, sowas kommt nur einmal
        ** oder soll immer angezeigt werden.
        ** -----------------------------------------------------------*/

        case LOGMSG_ROBODEAD:
        case LOGMSG_USERROBODEAD:
        case LOGMSG_ENEMYDESTROYED:
        case LOGMSG_LASTDIED:
        case LOGMSG_CREATED:
        case LOGMSG_FLAK_DESTROYED:
        case LOGMSG_RADAR_DESTROYED:
        case LOGMSG_POWER_ATTACK:
        case LOGMSG_RADAR_ATTACK:
        case LOGMSG_FLAK_ATTACK:

            ret = TRUE;

            break;

        /* ------------------------------------------------------------
        ** FoundEnemy wird außerhalb entschieden und kann hier auch
        ** immer durch. Mit einer Ausnahme: Die Roboflaks melden nicht,
        ** weil dies durch "Station under Attack" übernommen wird.
        ** ----------------------------------------------------------*/
        case LOGMSG_FOUNDENEMY:

            ret = TRUE;

            if( message->sender &&
                (BCLID_YPAGUN == message->sender->BactClassID) ) {

                ULONG rgun = 0L;
                _get( message->sender->BactObject, YGA_RoboGun, &rgun );
                if( rgun ) ret = FALSE;
                }

            break;

        /* ----------------------------------------------------------
        ** Bei Done extra Bearbeitung, weil die Messages, wenn man in
        ** einer Rakete sitzt, unterdrückt werden muß
        ** --------------------------------------------------------*/

        case LOGMSG_DONE:

            _get( yrd->world, YWA_UserVehicle, &uv );
            _get( uv, YBA_Bacterium, &bact );
            if( BCLID_YPAMISSY == bact->BactClassID )
                ret = FALSE;
            else
                ret = TRUE;

            break;
        }

    if( ret ) {

        /*** Parameter übernehmen ***/
        MessageArray[ message->ID ].para1 = message->para1;
        MessageArray[ message->ID ].para2 = message->para2;
        MessageArray[ message->ID ].para3 = message->para3;
        }

    return( ret );
}
