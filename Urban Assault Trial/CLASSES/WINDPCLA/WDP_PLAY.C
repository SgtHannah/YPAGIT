/*
**  $Source: $
**  $Revision: $
**  $Date: $
**  $Locker: $
**  $Author: $
**
**  Die DirectPlay-Sachen aus VFM-Sicht
**
*/
#include <exec/types.h>
#include <exec/memory.h>

#include "nucleus/nucleus2.h"
#include "network/windpclass.h"  // Public Version

#ifdef __NETWORK__

/*** Brot-O-Typen ***/
void  wdp_AskProviders( void *wdata );
char *wdp_GetProviderName( void *wdata, int number );
BOOL  wdp_SetProvider( void *wdata, char *name );
char *wdp_GetProvider( void *wdata );

BOOL  wdp_AskSessions( void *wdata );
char *wdp_GetSessionName( void *wdata, int number );
BOOL  wdp_JoinSession( void *wdata, char *name);
BOOL  wdp_OpenSession( void *wdata, char *name, ULONG maxplayers);
BOOL  wdp_CloseSession( void *wdata );
BOOL  wdp_GetSession( void *wdata, char **name, ULONG *status);
unsigned long wdp_GetSessionStatus( void *wdata );
BOOL  wdp_SetSessionName( void *wdata, char *name);

BOOL  wdp_CreatePlayer( void *wdata, char *name, ULONG f, ULONG d1, ULONG d2);
BOOL  wdp_DestroyPlayer( void *wdata, char *name);
BOOL  wdp_EnumPlayers( void *wdata );
BOOL  wdp_GetPlayerData( void *wdata, ULONG askmode, ULONG *number,
                         char **name, ULONG *data1, ULONG *data2, ULONG *flags);
ULONG wdp_AnnounceSendMessage( void *wdata, char *sender_id, ULONG sender_kind,
                               char *receiver_id, ULONG receiver_kind, char *data,
                               ULONG data_size, ULONG guaranteed );
ULONG wdp_CollectMessage( void *wdata, char *sender_id, ULONG sender_kind,
                          char *receiver_id, ULONG receiver_kind, char *data,
                          ULONG data_size, ULONG guaranteed );
ULONG wdp_FlushBuffer( void *wdata, char *sender_id, ULONG sender_kind,
                       char *receiver_id, ULONG receiver_kind, ULONG guaranteed );
unsigned long wdp_TryReceiveMessage( struct windp_win_data *wdata, char *data,
                                     char *sender_id, unsigned long *sender_kind,
                                     char *receiver_id,
                                     unsigned long *receiver_kind,
                                     unsigned long *message_kind, ULONG *size );
BOOL  wdp_GetCapsInfo( void *wdata, ULONG *maxbuffersize, ULONG *baud);
BOOL  wdp_LockSession( void *wdata, ULONG block);
BOOL  wdp_Reset( void *wdata);
BOOL  wdp_GetNumPlayers( void *wdata);
BOOL  wdp_CheckRemoteStart( void *wdata, char *name, UBYTE *is_host,
                            UBYTE *remote );
void wdp_SetVersionIdent(   struct windp_win_data *wdata, char *string );
unsigned long wdp_GetProviderType( struct windp_win_data *wdata );
void wdp_Diagnosis( struct windp_win_data *wdata, unsigned long *num_send,
                    unsigned long *num_recv, unsigned long *max_send,
                    unsigned long *max_recv, unsigned long *tme_send,
                    unsigned long *max_tme_send, unsigned long *num_bug_send );


/*-----------------------------------------------------------------
**                      P R O V I D E R
**---------------------------------------------------------------*/

_dispatcher( void, wdp_NWM_ASKPROVIDERS, void *nix )
{
    /* ----------------------------------------------------------------
    ** Providerabfrage. Die Deklaration des Hooks und die
    ** entsprechenden Routinen sind Windows-Spezifisch und laufen somit
    ** in der Box. Dies ist also nur die Schale für VFM
    ** --------------------------------------------------------------*/
    struct windp_data *wdp;

    wdp = INST_DATA(cl, o );

    wdp_AskProviders( (void *)(wdp->win_data) );
}


_dispatcher( BOOL, wdp_NWM_GETPROVIDERNAME, struct getprovidername_msg *gpn )
{
    /*
    ** Gibt den Namen des providers zurück, dessen Nummer übergeben wurde.
    ** Wenn dazu noch einer existiert, ist der Rückkehrwert TRUE, sonst
    ** FALSE. Eine Abfrage der Provider könnte so aussehen:
    **
    ** gpn.number = 0;
    ** while( _methoda( nwo, NWM_GETPROVIDERNAME, &gpn )) {
    **
    **      ausgabe( gpn.name );
    **      gpn.number++;
    **      }
    */

    char *n;
    struct windp_data *wdp;

    wdp = INST_DATA(cl, o );

    if( n = wdp_GetProviderName( wdp->win_data, gpn->number ) ) {

        gpn->name = n;
        return( TRUE );
        }
    else {

        gpn->name = NULL;
        return( FALSE );
        }
}


_dispatcher( BOOL, wdp_NWM_SETPROVIDER, struct setprovider_msg *sp )
{
    /* -------------------------------------------------------
    ** Setzen eines providers. Wieder voll DP-spezifisch. Also
    ** an die WinBox übergeben
    ** -----------------------------------------------------*/
    struct windp_data *wdp;

    wdp = INST_DATA(cl, o );

    if( wdp_SetProvider( wdp->win_data, sp->name ) )
        return( TRUE );
    else
        return( FALSE );
}


_dispatcher( BOOL, wdp_NWM_GETPROVIDER, struct getprovider_msg *gp )
{
    /* --------------------------------------------------------
    ** Ermittelt den Namen des aktuell eingestellten Providers,
    ** sofern dieser verfügbar ist (sonst eben NULL).
    ** -----------------------------------------------------*/
    struct windp_data *wdp;

    wdp = INST_DATA(cl, o );

    if( gp->name = wdp_GetProvider( wdp->win_data ) )
        return( TRUE );
    else
        return( FALSE );
}


_dispatcher( ULONG, wdp_NWM_GETPROVIDERTYPE, void *nix )
{
    /* -------------------------------------------------------
    ** In opposite to method before here comes a constant that
    ** describes the connection.
    ** -----------------------------------------------------*/
    struct windp_data *wdp;

    wdp = INST_DATA(cl, o );

    return( wdp_GetProviderType( wdp->win_data ) );
}
    
 
/*-----------------------------------------------------------------
**                      S E S S I O N S
**---------------------------------------------------------------*/


_dispatcher( BOOL, wdp_NWM_ASKSESSIONS, void *nix )
{
    /* ----------------------------------------------------------------
    ** Providerabfrage. Die Deklaration des Hooks und die
    ** entsprechenden Routinen sind Windows-Spezifisch und laufen somit
    ** in der Box. Dies ist also nur die Schale für VFM
    ** --------------------------------------------------------------*/
    struct windp_data *wdp;

    wdp = INST_DATA(cl, o );

    return( wdp_AskSessions( (void *)(wdp->win_data) ) );
}


_dispatcher( BOOL, wdp_NWM_GETSESSIONNAME, struct getsessionname_msg *gsn )
{

    char *n;
    struct windp_data *wdp;

    wdp = INST_DATA(cl, o );

    if( n = wdp_GetSessionName( wdp->win_data, gsn->number ) ) {

        gsn->name = n;
        return( TRUE );
        }
    else {

        gsn->name = NULL;
        return( FALSE );
        }
}


_dispatcher( BOOL, wdp_NWM_JOINSESSION, struct joinsession_msg *js )
{
    struct windp_data *wdp;

    wdp = INST_DATA(cl, o );

    return( wdp_JoinSession( wdp->win_data, js->name ) );
}


_dispatcher( BOOL, wdp_NWM_OPENSESSION, struct opensession_msg *os )
{
    struct windp_data *wdp;

    wdp = INST_DATA(cl, o );

    return( wdp_OpenSession( wdp->win_data, os->name, os->maxplayers ) );
}


_dispatcher( BOOL, wdp_NWM_CLOSESESSION, void *nix )
{
    struct windp_data *wdp;

    wdp = INST_DATA(cl, o );

    return( wdp_CloseSession( wdp->win_data ) );
}


_dispatcher( BOOL, wdp_NWM_GETSESSION, struct getsession_msg *gp )
{
    struct windp_data *wdp;

    wdp = INST_DATA(cl, o );

    return( wdp_GetSession( wdp->win_data, &(gp->name), &(gp->status)) );
}


_dispatcher( void, wdp_NWM_SETSESSIONIDENT, struct sessionident_msg *si )
{
    struct windp_data *wdp;

    wdp = INST_DATA(cl, o );
    
    wdp_SetVersionIdent( wdp->win_data, si->check_item );
}

_dispatcher( ULONG, wdp_NWM_GETSESSIONSTATUS, void *nix )
{
    struct windp_data *wdp;

    wdp = INST_DATA(cl, o );

    return( wdp_GetSessionStatus( wdp->win_data ) );
}

_dispatcher( BOOL, wdp_NWM_SETSESSIONNAME, struct setsessionname_msg *ssn )
{
    struct windp_data *wdp;

    wdp = INST_DATA(cl, o );

    return( wdp_SetSessionName( wdp->win_data, ssn->name ) );
}

    
/*-----------------------------------------------------------------
**                      P L A Y E R
**---------------------------------------------------------------*/

_dispatcher( BOOL, wdp_NWM_CREATEPLAYER, struct createplayer_msg *cp )
{
    struct windp_data *wdp;

    wdp = INST_DATA(cl, o );

    return( wdp_CreatePlayer( wdp->win_data, cp->name, cp->flags,
                              cp->data1, cp->data2 ) );
}


_dispatcher( BOOL, wdp_NWM_DESTROYPLAYER, struct destroyplayer_msg *dp )
{
    struct windp_data *wdp;

    wdp = INST_DATA(cl, o );

    return( wdp_DestroyPlayer( wdp->win_data, dp->name ) );
}


_dispatcher( BOOL, wdp_NWM_ENUMPLAYERS,  void *nix )
{
    /* -----------------------------------------------------------------
    ** Füllt das interne Array mit Informationen über gefundene Spieler
    ** OHNE RÜCKSICHT AUF VERLUSSTE. NUR AM ANFANG NACH OPEN/JOINSESSION
    ** ANWENDEN.
    ** ---------------------------------------------------------------*/
    struct windp_data *wdp;

    wdp = INST_DATA(cl, o );

    return( wdp_EnumPlayers( wdp->win_data ) );
}


_dispatcher( BOOL, wdp_NWM_GETPLAYERDATA, struct getplayerdata_msg *gpd )
{
    /* -----------------------------------------------------------------
    ** Gibt Daten zu dem Player zurück. Die daten können nach (interner)
    ** Nummer oder Namen gesucht werden. Der jeweils andere Parameter
    ** und der rest werden ausgefüllt.
    ** Ist der Player nicht verfügbar, kommt 0 zurück
    ** ---------------------------------------------------------------*/
    struct windp_data *wdp;

    wdp = INST_DATA(cl, o );

    return( wdp_GetPlayerData( wdp->win_data, gpd->askmode, &(gpd->number),
                               &(gpd->name), &(gpd->data1), &(gpd->data2),
                               &(gpd->flags) ) );
}


/* ------------------------------------------------------------------------
**                            M E S S A G E S
** ----------------------------------------------------------------------*/

_dispatcher( ULONG, wdp_NWM_SENDMESSAGE, struct sendmessage_msg *sm )
{
    /* ------------------------------------------------------------------
    ** Verschickt eine Message an jemanden. Ja. Hm. Genau. Vorerst machen
    ** wir es so, daß die MSG_ALL-Messages gesammelt werden. Dies ist
    ** aber Amok-spezifisch. Somit sollte da mal was in der Message
    ** vorbereitet werden.
    ** ----------------------------------------------------------------*/
    struct windp_data *wdp;

    wdp = INST_DATA(cl, o );

    if( sm->receiver_kind == MSG_ALL ) {

        return( wdp_CollectMessage( wdp->win_data, sm->sender_id, sm->sender_kind,
                                    sm->receiver_id, sm->receiver_kind,
                                    sm->data, sm->data_size, sm->guaranteed ) );
        }
    else {

        /* -----------------------------------------------------------------
        ** Die Message geht nur an einen Spieler. So kann es sein, dass
        ** die sich auf daten im Buffer bezieht. Folglich muessen wir diesen
        ** erstmal loeschen.
        ** ---------------------------------------------------------------*/
        wdp_FlushBuffer( wdp->win_data, sm->sender_id, sm->sender_kind,
                         NULL, MSG_ALL,
                         sm->guaranteed);

        return( wdp_AnnounceSendMessage( wdp->win_data, sm->sender_id, sm->sender_kind,
                                         sm->receiver_id, sm->receiver_kind,
                                         sm->data, sm->data_size, sm->guaranteed ) );
        }
}


_dispatcher( BOOL, wdp_NWM_RECEIVEMESSAGE, struct receivemessage_msg *rm )
{
    /* -----------------------------------------------------------------
    ** Empfängt Message
    ** ---------------------------------------------------------------*/
    struct windp_data *wdp;

    wdp = INST_DATA(cl, o );

    return((BOOL)wdp_TryReceiveMessage( wdp->win_data, &(rm->data), &(rm->sender_id),
                                        &(rm->sender_kind), &(rm->receiver_id),
                                        &(rm->receiver_kind), &(rm->message_kind),
                                        &(rm->size) ) );
}


_dispatcher( ULONG, wdp_NWM_FLUSHBUFFER, struct flushbuffer_msg *fb )
{
    /* ------------------------------------------------------------------
    ** Der Buffer für SendeDaten soll verschickt werden. Dabei übergeben
    ** wir erneut die Adresse. Das mag umständlich sein, aber so brauchen
    ** wir keine speziellen für Sender und Empfänger zu merken.
    ** Gibt anzahl gesendeter bytes oder 0 zurueck
    ** ----------------------------------------------------------------*/
    struct windp_data *wdp;

    wdp = INST_DATA(cl, o );

    return(wdp_FlushBuffer( wdp->win_data, fb->sender_id, fb->sender_kind,
                            fb->receiver_id, fb->receiver_kind,
                            fb->guaranteed ) );
}


_dispatcher( void, wdp_NWM_DIAGNOSIS, struct diagnosis_msg *dm )
{
    /* -----------------------------------------------------------------
    ** Gibt statistische Informationen ueber das Sendeverhalten zurueck:
    ** Wieviel sind zuletzt mit einem Mal gesendet/empfangen worden und
    ** was war das Maximum dazu.
    ** ---------------------------------------------------------------*/
    struct windp_data *wdp;

    wdp = INST_DATA(cl, o );

    wdp_Diagnosis( wdp->win_data , &(dm->num_send), &(dm->num_recv), 
                   &(dm->max_send), &(dm->max_recv), &(dm->tme_send), 
                   &(dm->max_tme_send), &(dm->num_bug_send) );
}


/* ---------------------------------------------------------------------
**                               D I V E R S E S
** -------------------------------------------------------------------*/

_dispatcher( BOOL, wdp_NWM_GETCAPSINFO, struct getcapsinfo_msg *gci )
{
    /* -----------------------------------------------------------------
    **
    ** ---------------------------------------------------------------*/
    struct windp_data *wdp;

    wdp = INST_DATA(cl, o );

    return( wdp_GetCapsInfo( wdp->win_data, &(gci->maxbuffersize), &(gci->baud)));
}


_dispatcher( BOOL, wdp_NWM_LOCKSESSION, struct locksession_msg *bs )
{
    /* -----------------------------------------------------------------
    ** Blockiert eine Session für das joinen
    ** ---------------------------------------------------------------*/
    struct windp_data *wdp;

    wdp = INST_DATA(cl, o );

    return( wdp_LockSession( wdp->win_data, bs->block ) );
}


_dispatcher( void, wdp_NWM_RESET, void *bs )
{
    /* -----------------------------------------------------------------
    ** Setzt das Objekt zurück für ein neues Spiel
    ** ---------------------------------------------------------------*/
    struct windp_data *wdp;

    wdp = INST_DATA(cl, o );

    wdp_Reset( wdp->win_data );
}


_dispatcher( ULONG, wdp_NWM_GETNUMPLAYERS, void *bs )
{
    /* -----------------------------------------------------------------
    ** Liefert die Anzahl der Spieler zurück
    ** ---------------------------------------------------------------*/
    struct windp_data *wdp;

    wdp = INST_DATA(cl, o );

    return( wdp_GetNumPlayers( wdp->win_data ) );
}


/* ---------------------------------------------------------------------
**                         F E R N S T E U E R U N G
** -------------------------------------------------------------------*/
_dispatcher( BOOL, wdp_NWM_CHECKREMOTESTART, struct checkremotestart_msg *rs )
{
    /* -----------------------------------------------------------------
    ** Testet, ob Informationen für eine Initialisierung und einen
    ** Fernstart übergeben wurden. Wenn dem so war, wird alles (inkl.
    ** Player) initialisiert und TRUE zurückgegeben. Die übergebene
    ** Message wird ausgefüllt.
    ** ---------------------------------------------------------------*/
    struct windp_data *wdp;

    wdp = INST_DATA(cl, o );

    return( wdp_CheckRemoteStart( wdp->win_data, rs->name, &(rs->is_host),
                                  &(rs->remote) ) );
}

#endif // von __NETWORK__
