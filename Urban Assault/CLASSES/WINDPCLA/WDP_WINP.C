/*
**  $Source: $
**  $Revision: $
**  $Date: $
**  $Locker: $
**  $Author: $
**
*/

#define WINDP_WINBOX            // itze wirds Box-Zeuch

#include "stdio.h"
#include "nucleus/sys.h"
#include "network/windpclass.h"  // Public Version
#include <winbase.h>

#ifdef __NETWORK__

/*** definiert in win_main.c, ausgefuellt einmalig hier ***/
extern HWND win_HWnd;

#define THREAD_STACK 64000
struct SRThread  srthread;
CRITICAL_SECTION critical_section;

void wdp_FreeMem( void *mem );

/* -------------------------------------------------------------
** NEU: Weil im Remoteaufruf nur dpo2 ausgef¸llt ist, sind alle
** Tests nur noch auf dpo2. Es ist ja auch das, was wir brauchen
**
** Ebenfalls neu: Ich steige hiermit um auf DP3, somit entfallen
** alle Providersachen. Das DP-Objekt kann roh erzeugt werden
** und dann werden alle Connections, was letztenendlich eine
** Uebermenge der Provider ist. Nach aussen hin lasse ich die
** Namen der Funktionen, wie sie sind. Das Einstellen eines pro-
** viders ist gleich, auch wenn sich dahinter Connections ver-
** bergen.
** -----------------------------------------------------------*/

/*-----------------------------------------------------------------*/
BOOL FAR PASCAL wdp_ProviderCallback( LPGUID lpspGUID, LPVOID Connection, DWORD ConnSize, LPCDPNAME lpszSPName,
                                      DWORD  Flags, LPVOID lpcontext );
BOOL FAR PASCAL wdp_SessionCallback( LPCDPSESSIONDESC2 lpSessionDesc,
                                     LPDWORD lpdwTimeOut,
                                     DWORD dwFlags,
                                     LPVOID lpcontext );
BOOL WINAPI PlayerCallback( DPID dpid, DWORD dwPlayerType, LPCDPNAME lpName,
                            DWORD flags, LPVOID lpContext );
LPVOID wdp_ErrorMessage( HRESULT hr, char *wo );
LPVOID wdp_StringMessage( char *t );
LPVOID wdp_NumberMessage( DWORD n );
unsigned long wdp_CloseSession( struct windp_win_data *wdata );
unsigned long wdp_PlayerIDFromName( struct windp_win_data *wdata, char *name );
unsigned long wdp_PlayerOffsetFromName( struct windp_win_data *wdata, char *name );
char *wdp_PlayerNameFromID( struct windp_win_data *wdata, unsigned long dpid );
unsigned long wdp_EnumPlayers( struct windp_win_data *wdata );
unsigned long wdp_DestroyPlayer( struct windp_win_data *wdata, char *name );
unsigned long wdp_GetPlayerData( struct windp_win_data *wdata, unsigned long askmode,
                                 unsigned long *number, char **name,
                                 unsigned long *data1, unsigned long *data2,
                                 unsigned long *flags);
unsigned long wdp_AskSessions( struct windp_win_data *wdata );
unsigned long wdp_CheckRemoteStart( void *wdata, char *name, unsigned char *is_host,
                                    unsigned char *remote );
void wdp_SetGuaranteedMode( struct windp_win_data *wdata, unsigned long data );
void wdp_SetVersionCheck(   struct windp_win_data *wdata, BOOL data );
void wdp_SetDebug(          struct windp_win_data *wdata, BOOL data );
void wdp_SetVersionIdent(   struct windp_win_data *wdata, char *string );
unsigned long wdp_GetProviderType( struct windp_win_data *wdata );
unsigned long wdp_SRThread( void *t );
unsigned long wdp_OpenLogScript(void);
unsigned long wdp_Log( char* string, ... );
void *wdp_AllocMem( unsigned long size );
unsigned long yw_Startprofile( void );
unsigned long yw_Endprofile( unsigned long t );


/* --------------------------------------------------------------------
**                        P R O V I D E R
** ------------------------------------------------------------------*/


unsigned long wdp_AskProviders( struct windp_win_data *wdata )
{
    /* ------------------------------------------------
    ** Diese Routine erfragt die Connections, welche
    ** der namensgleichheit wegen provider heissen.
    ** Das Objekt sollte schon in OM_NEW erzeugt worden
    ** sein     
    ** ----------------------------------------------*/
    HRESULT hr;
    GUID    ypa_guid;
    
    if( !(wdata->dpo2)) return( 0L );
    
    /*** Initkram ***/
    wdata->num_providers = 0;
    ypa_guid = YPA_GUID;
    
    hr = wdata->dpo2->lpVtbl->EnumConnections( wdata->dpo2, &ypa_guid, wdp_ProviderCallback, wdata, 0 );
                                               //DPCONNECTION_DIRECTPLAY|DPCONNECTION_DIRECTPLAYLOBBY );

    /*** any problems ? ***/
    if( DP_OK != hr ) {
        
        wdp_ErrorMessage( hr,"AskProviders/EnumConnections" );
        return( 0L );
        }
        
    return( 1L );    
}


BOOL FAR PASCAL wdp_ProviderCallback( LPGUID    lpspGUID,
                                      LPVOID    lpConnection,
                                      DWORD     ConnSize,  
                                      LPCDPNAME lpName,
                                      DWORD     Flags,
                                      LPVOID    lpcontext )
{
    /* --------------------------------------------------------------
    ** Diese HockRoutine wird f¸r jeden provider einmal aufgerufen.
    ** Das heiﬂt f¸r uns, daﬂ wir uns in das Provider-Array eintragen
    ** und den internen Z‰hler eins hochsetzen.Uebergeben wird eine
    ** Connection, die wir aufheben muessen.
    ** ------------------------------------------------------------*/
    struct windp_win_data *wdata;
    HRESULT hr;
    LPDIRECTPLAY3A  temp_dpo;

    wdata = (struct windp_win_data *) lpcontext;

    /*** Wenn das Array voll ist, ignorieren wir die Sache ***/
    if( wdata->num_providers == MAXNUM_PROVIDERS )
        return( FALSE );
        
    /*** name merken ***/
    memset(  wdata->provider[ wdata->num_providers ].name, 0, PROVIDER_NAMELEN );
    strncpy( wdata->provider[ wdata->num_providers ].name,
             lpName->lpszShortNameA, PROVIDER_NAMELEN );
    
    /*** ok, die GUID naturlich auch ***/
    memcpy(  (void *) &(wdata->provider[ wdata->num_providers ].guid),
             (void *) lpspGUID, sizeof( GUID ) );
             
    /*** Connections merken, das ist neu in DPlay3 ***/
    wdata->provider[ wdata->num_providers ].connection = NULL;
    wdata->provider[ wdata->num_providers ].connection = malloc( ConnSize );
    if( NULL == wdata->provider[ wdata->num_providers ].connection ) 
        return( FALSE );                          
    memcpy(  (void *) wdata->provider[ wdata->num_providers ].connection,
             (void *) lpConnection, ConnSize );
             
    /* ----------------------------------------------------------------------------------------
    ** test, ob das auch wirklich klappt. dazu erzeuge ich ein neues Objekt und gebe es nachher
    ** wieder frei, um keine ALREADYITINIALISED-Messages zu bekommen. 
    ** --------------------------------------------------------------------------------------*/
    temp_dpo = NULL;
    hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlay3A, &(temp_dpo) );
    if( DP_OK == hr ) {
    
        hr = temp_dpo->lpVtbl->InitializeConnection( temp_dpo, lpConnection, 0 );
        
        if( DP_OK == hr ) {
        
            /*** laesst sich setzen, folglich ok ***/       
            wdata->num_providers++;
            }
        
        temp_dpo->lpVtbl->Release(temp_dpo);    
        }         
             
    return( TRUE );
}


char *wdp_GetProviderName( struct windp_win_data *wdata, int number )
{
    /* -------------------------------------------------------------
    ** Diese Routine erfragt den Namen des number-ten Providers. Ist
    ** dieser nicht verf¸gbar, kommt NULL zur¸ck.
    ** -----------------------------------------------------------*/
    if( number >= wdata->num_providers )
        return( NULL );
    else
        return( wdata->provider[ number ].name );
}


unsigned long wdp_SetProvider( struct windp_win_data *wdata, char *name )
{
    /* --------------------------------------------------------------------------
    ** Hier gibt es nun eine Menge Aenderungen. Es wird kein Objekt mehr erzeugt, 
    ** sondern nur noch der provider, auf DP3-deutsch: Die Connection, gesetzt.
    ** Die ConnectionInfos stehen im providerfeld und wurden vorher abgefragt.
    ** Achtung, macht jetzt auch AskSessions!
    ** ------------------------------------------------------------------------*/

    HRESULT hr;
    struct  provider_entry *prov;
    int     count;
    DPCAPS  caps;
    
    /*** in dieses File gehen nun alle fehlermeldungen....***/
    if( wdata->debug ) 
        wdp_OpenLogScript();    

    /*** Provider ermitteln ***/
    count = 0;
    prov  = NULL;
    while( count < wdata->num_providers ) {

        if( stricmp( wdata->provider[ count ].name, name ) == 0 ) {

            /*** Er ist es! Lasset uns ihn preisen! ***/
            prov = &( wdata->provider[ count ] );
            break;
            }

        count++;
        }

    /*** Nix gefunden ? ***/
    if( !prov ) return( 0L );
    
    /* ----------------------------------------------------------------------------
    ** Das Arbeiten hiermit ist, wie sich selbst am Zopf aus dem Sumpf zu ziehen.
    ** Erst Objekt erzeugen, dann Connections erfragen und dann zum Setzen Objekt
    ** vorher loeschen und wieder erzeugen, weil auch ein missglueckter Setzversuch
    ** das Objekt unbrauchbar macht. Ohne worte...
    ** --------------------------------------------------------------------------*/
    wdata->dpo2->lpVtbl->Release( wdata->dpo2 );
    wdata->dpo2 = NULL;
    hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlay3A, &(wdata->dpo2) );
    if( hr != DP_OK ) {
    
        wdp_ErrorMessage( hr,"SetProvider/CoCreateInstance" );
        return( 0L );
        }

    /*** nun die Connection setzen ***/
    hr = wdata->dpo2->lpVtbl->InitializeConnection( wdata->dpo2, (LPVOID)(prov->connection), 0 );
        
    if( hr != DP_OK ) {
    
        wdp_ErrorMessage( hr, "SetProvider/InitializeConnection" );
        wdata->conn_type = NWFC_NONE;
        return( 0L );
        }
        
    if( IsEqualGUID( &(prov->guid), &(DPSPGUID_TCPIP) ) )
        wdata->conn_type = NWFC_TCPIP;
    else                
        if( IsEqualGUID( &(prov->guid), &(DPSPGUID_IPX) ) )
            wdata->conn_type = NWFC_IPX;
        else                
            if( IsEqualGUID( &(prov->guid), &(DPSPGUID_SERIAL) ) )
                wdata->conn_type = NWFC_SERIAL;
            else                
                if( IsEqualGUID( &(prov->guid), &(DPSPGUID_MODEM) ) )
                    wdata->conn_type = NWFC_MODEM;
                else                
                    wdata->conn_type = NWFC_NONE;
   

    wdata->session_joined = wdata->session_created = 0L;
    wdata->num_players    = wdata->num_sessions    = 0;

    /* ----------------------------------------------------------
    ** Gleich mit abfragen, wenn kein Modem. Seriell schon, wegen
    ** Verbindungsdaten 
    ** --------------------------------------------------------*/
    if( wdata->conn_type != NWFC_MODEM )
        if( !wdp_AskSessions( wdata ) ) return( 0L );

    /*** Alles ok ***/
    wdata->act_provider = count;

    /*** Caps abfragen. Brauchen wir das noch? ***/
    ZeroMemory( &caps, sizeof( DPCAPS ) );
    caps.dwSize = sizeof( DPCAPS );
    hr = wdata->dpo2->lpVtbl->GetCaps( wdata->dpo2, &caps, 0 );
    if( DP_OK == hr ) {

        if( caps.dwFlags & DPCAPS_GUARANTEEDSUPPORTED )
            wdata->guaranteed = 1;
        else
            wdata->guaranteed = 0;
        }
    else wdp_ErrorMessage( hr, "SetProvider/GetCaps" );
    
    return( 1L );
}


char *wdp_GetProvider( struct windp_win_data *wdata )
{
    if( wdata->dpo2 ) {

        /* ------------------------------------------------------
        ** Die objekte sind ausgef¸llt und wir kˆnnen was zur¸ck-
        ** geben.
        ** ----------------------------------------------------*/
        return( wdata->provider[ wdata->act_provider ].name );
        }
    else {

        /* ----------------------------------------------------------
        ** Es war noch nicht s initialisiert oder die Initialisierung
        ** ging schief...
        ** --------------------------------------------------------*/
        return( NULL );
        }
}


unsigned long wdp_GetProviderType( struct windp_win_data *wdata )
{
    /* -----------------------------------------------------------
    ** Gibt den Typ des providers zurueck, welcher bei Initialize-
    ** Connection gestezt wird.
    ** ---------------------------------------------------------*/
    
    return( (unsigned long) (wdata->conn_type) );
}
     

/* --------------------------------------------------------------------
**                        S E S S I O N S
** ------------------------------------------------------------------*/


unsigned long wdp_AskSessions( struct windp_win_data *wdata )
{
    /* -------------------------------------------------------
    ** Erfragt die Sessions zum eingestellten provider und der
    ** YPA-GUID.
    ** Damit wird dann das Array ausgef¸llt, aber das macht eh
    ** der Hook.
    ** Neu: Asnchroner Aufruf, dieser wird bei OpenSession von
    ** DP automatisch zur¸ckgesetzt
    ** -----------------------------------------------------*/
    DPSESSIONDESC2 sessiondesc;
    HRESULT hr;
    DWORD   flags;

    ZeroMemory( &sessiondesc, sizeof( DPSESSIONDESC2 ) );
    sessiondesc.dwSize          = sizeof( DPSESSIONDESC2 );
    sessiondesc.guidApplication = YPA_GUID;

    wdata->num_sessions = 0;
    
    /* ---------------------------------------------------------------
    ** Wegen den problemen (GENERIC ERROR) der seriellen Schnittstelle
    ** vorerst hier keine Asynchrone Suche 
    ** -------------------------------------------------------------*/
    
    flags = DPENUMSESSIONS_AVAILABLE; 
    if( wdata->conn_type != NWFC_SERIAL )
        flags |= DPENUMSESSIONS_ASYNC;
        
    hr = wdata->dpo2->lpVtbl->EnumSessions( wdata->dpo2,
                                            &sessiondesc,
                                            0,
                                            wdp_SessionCallback,
                                            (LPVOID) wdata,
                                            flags );

    if( DPERR_USERCANCEL == hr )
        return( 0L );

    if( (hr != DP_OK) && (hr != DPERR_USERCANCEL) ) {

        /* --------------------------------------------------------------
        ** da wir asynchrone Suche nehmen und bei DPlay ueber serial line
        ** probleme auftreten, wenn die eine box nicht vor der anderen
        ** geschlossen wird, das aber keine Probleme zu machen scheint,
        ** ignoriere ich diesen fehler  mal...
        ** ------------------------------------------------------------*/

        if( !( (wdata->conn_type == NWFC_SERIAL) && (hr == DPERR_GENERIC) )) {
            
            /*** Krieg ma nich ***/     
            wdp_ErrorMessage( hr, "AskSessions/EnumSessions" );
            return( 0L );
            }
        }

    return( 1L );
}


BOOL FAR PASCAL wdp_SessionCallback( LPDPSESSIONDESC2 lpSessionDesc,
                                     LPDWORD lpdwTimeOut,
                                     DWORD dwFlags,
                                     LPVOID lpcontext )
{
    struct windp_win_data *wdata = (struct  windp_win_data *) lpcontext;

    /*** Keine R¸ckmeldung. Kamma n¸scht machen ***/
    if( dwFlags & DPESC_TIMEDOUT )
        return( FALSE );

    /*** Wenn das Array voll ist, ignorieren wir die Sache ***/
    if( wdata->num_sessions == MAXNUM_SESSIONS )
        return( FALSE );
        
    /* -----------------------------------------------------------------
    ** Wenn eine Ueberpruefung gewuenscht wird, fallen alle die Sessions
    ** raus, in deren Namen nicht der IDENT auftaucht
    ** ---------------------------------------------------------------*/
    if( wdata->versioncheck ) {
    
        if( !strstr( lpSessionDesc->lpszSessionNameA, wdata->versionident ) )
            return( TRUE ); // er soll ja eiter suchen
        }        

    /*** Verbotene Sessions mal per Hand rausfiltern ***/
    if( lpSessionDesc->dwFlags & DPSESSION_JOINDISABLED )
        return( TRUE );

    /*** Die Daten zur Session speichern wir in einer Struktur ***/
    memset(  wdata->session[ wdata->num_sessions ].name, 0, SESSION_NAMELEN );
    if( lpSessionDesc->lpszSessionNameA )
        strncpy( wdata->session[ wdata->num_sessions ].name,
                 lpSessionDesc->lpszSessionNameA, SESSION_NAMELEN );

    memset(  wdata->session[ wdata->num_sessions ].password, 0, SESSION_NAMELEN );
    if( lpSessionDesc->lpszPasswordA )
        strncpy( wdata->session[ wdata->num_sessions ].password,
                 lpSessionDesc->lpszPasswordA, SESSION_NAMELEN );

    memcpy(  (void *) &(wdata->session[ wdata->num_sessions ].guid),
             (void *) &(lpSessionDesc->guidInstance), sizeof( GUID ) );

    /*** Zum Joinen merke ich mir die gesamte Sessiondesc ***/
    memcpy(  (void *) &(wdata->session[ wdata->num_sessions ].desc),
             (void *) lpSessionDesc, sizeof( DPSESSIONDESC2 ) );

    wdata->num_sessions++;
    return( TRUE );
}


char *wdp_GetSessionName( struct windp_win_data *wdata, int number )
{
    /* -----------------------------------------------------------
    ** Diese Routine erfragt den Namen der number-ten Session. Ist
    ** diese nicht verf¸gbar, kommt NULL zur¸ck.
    ** ---------------------------------------------------------*/
    if( number >= wdata->num_sessions )
        return( NULL );
    else
        return( wdata->session[ number ].name );
}


unsigned long wdp_OpenSession( struct windp_win_data *wdata, char *name,
                               unsigned long maxplayers )
{
    /* -------------------------------------------------------
    ** Versucht eine Session zu ˆffnen. War dies erfoolgreich,
    ** kommt TRUE zur¸ck.
    ** Achtung, erfragt auch gleich die Player mit
    ** -----------------------------------------------------*/
    HRESULT hr;
    DPSESSIONDESC2 sessiondesc;

    /*** Aufr‰umen ***/
    if( wdata->session_created || wdata->session_joined )
        wdp_CloseSession( wdata );

    /*** Vorbereitungen ***/
    ZeroMemory( &sessiondesc, sizeof( DPSESSIONDESC2 ) );
    sessiondesc.dwSize  = sizeof( DPSESSIONDESC2 );
    sessiondesc.dwFlags = DPSESSION_MIGRATEHOST | DPSESSION_KEEPALIVE;
    sessiondesc.guidApplication  = YPA_GUID;
    sessiondesc.dwMaxPlayers     = maxplayers;
    sessiondesc.lpszSessionNameA = name;

    /*** itze giehts los! ***/
    if( !(wdata->dpo2) ) return( 0L );

    hr = wdata->dpo2->lpVtbl->Open( wdata->dpo2,
                                    &sessiondesc,
                                    DPOPEN_CREATE );

    if( hr != DP_OK ) {
        
        if( hr != DPERR_USERCANCEL )
            wdp_ErrorMessage( hr, "OpenSession/Open" );
        return( 0L );
        }

    /*** Daten zur aktuell eingestellten Session merken ***/
    strcpy( wdata->own_session.name, name );
    wdata->own_session.guid = sessiondesc.guidInstance;
    wdata->session_created  = 1L;
    wdata->session_joined   = 0L;

    /*** Itze mit... ***/
    wdp_EnumPlayers( wdata );

    return( 1L );
}


unsigned long wdp_JoinSession( struct windp_win_data *wdata, char *name )
{
    /* ----------------------------------------------------------
    ** Versucht, in eine Session zu kommen. War dies erfolgreich,
    ** kommt TRUE zur¸ck.
    ** --------------------------------------------------------*/
    HRESULT hr;
    DPSESSIONDESC2 sessiondesc;
    unsigned long i, found = 0L, offset;

    /*** Aufr‰umen ***/
    if( wdata->session_created || wdata->session_joined )
        wdp_CloseSession( wdata );

    /*** Session suchen im internen Array ***/
    i = wdata->num_sessions;
    while( i ) {

        if( stricmp( wdata->session[ i - 1 ].name, name ) == 0 ) {
            offset = i - 1;  found = 1L; break; }
        i--;
        }

    if( !found ) return( 0L );

    /*** Session moit den mir bekannten daten vorbereiten ***/
    ZeroMemory( &(sessiondesc), sizeof( DPSESSIONDESC2) );
    sessiondesc.dwSize = sizeof( DPSESSIONDESC2 );
    sessiondesc.guidInstance    = wdata->session[ offset ].guid;
    sessiondesc.guidApplication = YPA_GUID;

    /*** itze giehts los! ***/
    if( !(wdata->dpo2) ) return( 0L );

    hr = wdata->dpo2->lpVtbl->Open( wdata->dpo2,
                                    &(sessiondesc),
                                    DPOPEN_JOIN );

    if( hr != DP_OK ) {

        if( hr != DPERR_USERCANCEL )
            wdp_ErrorMessage( hr, "OpenSession/join" );
            
        /* ---------------------------------------------------
        ** Im Modemfalle sessions loeschen, weil man nicht neu
        ** fragen kann, ohne den Requester zu bringen.
        ** -------------------------------------------------*/
        if( wdata->conn_type == NWFC_MODEM )
            wdata->num_sessions = 0;
            
        return( 0L );
        }

    /*** Daten zur aktuell eingestellten Session merken ***/
    wdata->session_joined  = 1L;
    wdata->session_created = 0L;
    wdata->act_session     = offset;

    /*** Itze mit... ***/
    wdp_EnumPlayers( wdata );

    return( 1L );

}


unsigned long wdp_CloseSession( struct windp_win_data *wdata )
{
    /* -------------------------------------------------------
    ** Schlieﬂt die aktuelle Session. Dabei wird vorerst nicht
    ** (zumindest f¸r die DP-Seite) unterschieden, wer die
    ** Session geˆffnet hat.
    ** -----------------------------------------------------*/
    HRESULT hr;
    unsigned long data1, data2, flags, number;
    char     *n;

    if( !(wdata->dpo2) ) return( 0L );

    if( !(wdata->session_created || wdata->session_joined )) {
    
        return( 0L );
        }
        
    /*** Wenn noch player von mir da sind, killen ***/
    number = 0;
    while( wdp_GetPlayerData( wdata, GPD_ASKNUMBER, &number, &n,
                              &data1, &data2, &flags) ) {

        if( flags & WDPF_OWNPLAYER )
            wdp_DestroyPlayer( wdata, n );
        else
            number++;
        }

    hr = wdata->dpo2->lpVtbl->Close( wdata->dpo2 );

    /*** in meinem Zeug noch r¸cksetzen ***/
    wdata->session_created = 0L;
    wdata->session_joined  = 0L;

    if( hr != DP_OK ) {

        wdp_ErrorMessage( hr,"CloseSession/Close" );
        return( 0L );
        }
            
    return( 1L );

}


unsigned long wdp_GetSessionStatus( struct windp_win_data *wdata )
{
    if( wdata->session_joined )
        return( WSS_SESSION_JOINED );
    else
        if( wdata->session_created )
            return( WSS_SESSION_CREATED );
        else
            return( WSS_SESSION_NONE );
}


unsigned long wdp_SetSessionName( struct windp_win_data *wdata, char *name )
{            
    HRESULT hr;
    LPDPSESSIONDESC2 desc;
    DWORD   size;

    if( !(wdata->dpo2) ) return( 0L );

    /*** Ist ¸berhaupt ne Session dazu eingestellt? ***/
    if( !( wdata->session_joined || wdata->session_created) ) return( 0L );

    /*** Grˆﬂe holen, die grˆﬂer als DESC2 ist. Komisch... ***/
    hr = wdata->dpo2->lpVtbl->GetSessionDesc( wdata->dpo2, NULL, &size );
    if( (hr != DP_OK) && (hr != DPERR_BUFFERTOOSMALL) ) {

        wdp_ErrorMessage( hr, "SetSessionName/GetSessionDesc ask size" );
        return( 0L );
        }

    /*** Speicher holen. Passiert ja recht selten ***/
    desc = malloc( size );

    /*** Sessiondesc holen ***/
    hr = wdata->dpo2->lpVtbl->GetSessionDesc( wdata->dpo2, desc, &size );
    if( hr != DP_OK ) {

        wdp_ErrorMessage( hr,"SetSessionName/GetSessionDesc get msg" );
        free( desc );
        return( 0L );
        }

    /*** Name ausfuellen ***/
    desc->lpszSessionNameA = name;
    
    hr = wdata->dpo2->lpVtbl->SetSessionDesc( wdata->dpo2, desc, 0 );
    if( hr != DP_OK ) {

        wdp_ErrorMessage( hr, "SetSessionName/SetSessionDesc" );
        free( desc );
        return( 0L );
        }

    free( desc );
    return( 1L );
}

unsigned long wdp_GetSession( struct windp_win_data *wdata, char **name,
                              unsigned long *status )
{
    /* -----------------------------------------------------------
    ** Diese Routine erfragt den Namen der aktuellen Session. Ist
    ** diese nicht verf¸gbar, kommt NULL zur¸ck.
    ** ---------------------------------------------------------*/
    if( wdata->session_joined ) {

        *status = SESSION_JOINED;
        *name = wdata->session[ wdata->act_session ].name;
        return( 1L );
        }

    if( wdata->session_created ) {

        *status = SESSION_CREATED;
        *name = wdata->own_session.name;
        return( 1L );
        }

    *status = SESSION_NONE;
    return( 0L );
}


/* --------------------------------------------------------------------
**                          P L A Y E R
** ------------------------------------------------------------------*/

unsigned long wdp_CreatePlayer( struct windp_win_data *wdata, char *name,
                                unsigned long flags, DPID d1, unsigned long d2 )
{
    /* ---------------------------------------------------------------------
    ** Versucht, einen neuen Player zu erzeugen. Dabei testen wir zuerst, ob
    ** es den bei uns schon gibt. Auch wenn diverse applikationen wie
    ** das PHƒNOMENALE Y.P.A. nur einen Spieler pro Maschine verwalten
    ** werden, so biete ich trotzdem die Mˆglichkeit mehrerer Spieler
    ** an. Wenn es den im Netz schon gibt, dann wird DP schon meckern!
    ** -------------------------------------------------------------------*/
    DPNAME  dpName;
    HRESULT hr;
    long    i;

    if( !(wdata->dpo2) ) return( 0L );

    /*** Sind es schon zuviele? ***/
    if( wdata->num_players == MAXNUM_PLAYERS ) return( 0L );

    /*** Gibts den schon bei uns? ***/
    i = wdata->num_players;
    while( i ) {

        if( stricmp( wdata->player[ i - 1 ].name, name ) == 0 ) return( 0L );
        i--;
        }

    /*** Wirklich voll und ganz erzeugen? ***/
    if( flags & WDPF_OWNPLAYER ) {

        /*** Na, dann wollen wir mal den Windows-Kram erledigen ***/
        ZeroMemory( &dpName, sizeof( DPNAME) );
        dpName.dwSize         = sizeof( DPNAME );
        dpName.lpszShortNameA = name;
        dpName.lpszLongNameA  = NULL;

        /*** EventObject erzeugen ***/
        wdata->mevent = CreateEvent( 0, 0, 0, "YPAMESSAGE" );
        if( !wdata->mevent ) {

            wdp_StringMessage("cannot create Event for Receiving Messages");
            return( 0L );
            }

        hr = wdata->dpo2->lpVtbl->CreatePlayer( wdata->dpo2,
             &(wdata->player[ wdata->num_players ].dpid),
             &(dpName),
             wdata->mevent, NULL, 0, 0 );

        if( hr != DP_OK ) {

            wdp_ErrorMessage( hr,"CreatePlayer/CreatePlayer" );
            return( 0L );
            }

        wdata->player[ wdata->num_players ].flags |= WDPF_OWNPLAYER;

        /*** Sende- und Empfangsthread anlegen ***/

        memset( &srthread, 0, sizeof( srthread ) );
        srthread.win_data    = wdata;
        srthread.send_buffer = malloc( WDP_NORMALBLOCKSIZE );
        srthread.recv_buffer = malloc( WDP_NORMALBLOCKSIZE );
        srthread.thread = CreateThread( NULL, THREAD_STACK,
                                        (LPTHREAD_START_ROUTINE)wdp_SRThread,
                                        &srthread, 0, &srthread.threadid );
        if( !srthread.thread ) {

            wdp_StringMessage("Unable to create thread for dplay\n");
            return( 0L );
            }
        }
    else {

        /*** nur bei mir anlegen ***/
        wdata->player[ wdata->num_players ].flags &= ~WDPF_OWNPLAYER;
        wdata->player[ wdata->num_players ].dpid   = d1;
        wdata->player[ wdata->num_players ].data   = d2;
        }

    strcpy( wdata->player[ wdata->num_players ].name, name );

    /*** Na, das scheint ja geklappt zu haben ***/
    wdata->num_players++;

    return( 1L );
}


unsigned long wdp_DestroyPlayer( struct windp_win_data *wdata, char *name )
{
    /* ------------------------------------------------------------------
    ** Wir lˆschen den Player. Dazu melden wir ihn, sofern er bei uns
    ** ¸berhaupt registriert ist, bei DPlay ab und, jetzt wirds e bissel
    ** albern, weil wir nun mal ein Array haben, schieben die daten nach.
    ** ----------------------------------------------------------------*/

    unsigned long i, found = 0L, k, p;
    HRESULT hr;

    if( !(wdata->dpo2) ) return( 0L );

    /*** Gibts den bei uns? ***/
    i = wdata->num_players;
    while( i ) {

        if( stricmp( wdata->player[ i - 1 ].name, name ) == 0 ) {
            found = 1L; p = i - 1; break; }
        i--;
        }

    if( !found ) {
    
        return( 0L );
        }

    /*** Von DPlay verabschieden, wenn es ein eigener war ***/
    if( wdata->player[ p ].flags & WDPF_OWNPLAYER ) {

        /* ---------------------------------------------------------
        ** Thread wieder abmelden, dazu setze ich das Flag und wecke
        ** net Thread mittels SetEvent auf. 
        ** -------------------------------------------------------*/
        srthread.quit = 1L;
        Sleep( 30 );

        SetEvent( srthread.win_data->mevent );
        
        /*** auf abmeldung warten ***/
        WaitForSingleObject(srthread.thread, INFINITE);

        hr = wdata->dpo2->lpVtbl->DestroyPlayer( wdata->dpo2,
                    (DPID) wdata->player[ p ].dpid );
                    
        /*** Handles fuer Event und Thread freigeben ***/
        CloseHandle( srthread.win_data->mevent );
        CloseHandle( srthread.thread );

        /*** Fehler beeinfluﬂt nicht die interne Freigabe, oder? ***/
        if( hr != DP_OK ) wdp_ErrorMessage( hr, "DestroyPlayer/DestroyPlayer" );
        }

    /*** So, jetzt noch die internen daten Handeln ***/
    for( k = p; k < wdata->num_players - 1; k++ ) {

        wdata->player[ k ] = wdata->player[ k + 1 ];
        }
    wdata->num_players--;

    return( 1L );
}


unsigned long wdp_EnumPlayers( struct windp_win_data *wdata )
{
    /* --------------------------------------------------------------
    ** Ermittelt unter Nutzung der DPlay-Routinen die Ermittlung
    ** aller bereits erzeugten Player. Weil diese Routine das interne
    ** Array ¸berschreibt, darf sie nur nach dem Eintritt in eine
    ** Session aufgerufen werden.
    ** ------------------------------------------------------------*/

    HRESULT hr;

    if( !(wdata->dpo2) ) return( 0L );

    wdata->num_players = 0;

    hr = wdata->dpo2->lpVtbl->EnumPlayers( wdata->dpo2, NULL,
                                           PlayerCallback, wdata, 0 );
    if( hr != DP_OK ) {

        wdp_ErrorMessage( hr,"EnumPlayers/EnumPlayers" );
        return( 0L );
        }

    return( 1L );
}


BOOL WINAPI PlayerCallback( DPID dpid, DWORD dwPlayerType, LPCDPNAME lpName,
                            DWORD flags, LPVOID lpContext )
{
    struct windp_win_data *wdata = (struct windp_win_data *) lpContext;

    /*** Kˆnnen wir noch jemanden aufnehmen ***/
    if( wdata->num_players == MAXNUM_PLAYERS ) return( FALSE );

    /*** Hat sich da trotz allem eine Gruppe durchgeschl‰ngelt? ***/
    if( dwPlayerType == DPPLAYERTYPE_GROUP ) return( FALSE );

    /*** merken. Es sind alle ausw‰rtige Spieler! ***/
    wdata->player[ wdata->num_players ].flags &= ~NWFP_OWNPLAYER;
    wdata->player[ wdata->num_players ].dpid   = dpid;
    wdata->player[ wdata->num_players ].data   = 0L;
    strncpy( wdata->player[ wdata->num_players ].name,
            lpName->lpszShortNameA, PLAYER_NAMELEN );

    wdata->num_players++;

    return( TRUE );
}


unsigned long wdp_GetPlayerData( struct windp_win_data *wdata, unsigned long askmode,
                                 unsigned long *number, char **name,
                                 unsigned long *data1, unsigned long *data2,
                                 unsigned long *flags)
{
    struct player_entry *pl;
    unsigned long        offset;

    /*** Player suchen ***/
    if( askmode == GPD_ASKNAME ) {

        unsigned long i = wdata->num_players, found = 0L;

        while( i ) {

            if( stricmp( wdata->player[ i - 1 ].name, *name ) == 0 ) {
                offset = i - 1;  found  = 1L;  break; }
            i--;
            }

        if( !found ) return( 0L );
        }
    else {

        if( *number >= wdata->num_players ) return( 0L );
        offset = *number;
        }

    pl = &(wdata->player[ offset ]);

    /*** Nun Daten ausf¸llen ***/
    *number = offset;
    *name   = pl->name;
    *data1  = (unsigned long) pl->dpid;
    *data2  = pl->data;
    *flags  = pl->flags;

    return( 1L );
}


/* --------------------------------------------------------------------
**                          M E S S A G E S
** ------------------------------------------------------------------*/

unsigned long wdp_AnnounceSendMessage( struct windp_win_data *wdata, char *sender_id,
                               unsigned long sender_kind, char *receiver_id,
                               unsigned long receiver_kind, char *data,
                               unsigned long data_size, unsigned long guaranteed )
{
    /* ---------------------------------------------------------------------
    ** Kapselroutine fuer Send. Die Kapselung durch CollectMessage bleibt
    ** erhalten. Lediglich richtige, finale Sendaufrufe werden nicht mehr
    ** direkt ausgefuehrt, sondern angemeldet, so dass ein separater
    ** Thread, mit dem wir uns die srthread-Struktur teilen, diesen parallel
    ** ausfuehren kann.
    ** Hier werden wir kapseln, dass, wenn der 2. Sendaufruf kommt, obwohl
    ** der erste noch nicht bearbeitet ist, ein 2er Buffer genommen wird.
    **
    ** Weil Sendaufrufe auch ohne Collect und damit ohne sendbuffer erledigt
    ** werden koennen, nehmen wir einen eigenen Buffer;
    ** -------------------------------------------------------------------*/

    /*** Eine Node allozieren ***/
    unsigned long node_size;
    struct message_node *mn;

    node_size = sizeof( struct message_node ) + data_size;
    node_size += 32;     // nur zur Sicherheit
    mn = wdp_AllocMem( node_size );
    if( mn ) {

        memset( mn, 0, node_size );
        if( sender_id )
            strcpy( mn->sender_id, sender_id );
        if( receiver_id )
            strcpy( mn->receiver_id, receiver_id );
        mn->sender_kind   = sender_kind;
        mn->receiver_kind = receiver_kind;
        mn->recv_size     = data_size;
        mn->guaranteed    = guaranteed;
        memcpy( &(mn->data), data, data_size );

        /*** Node anhaengen ***/
        EnterCriticalSection( &critical_section );
        win_AddTail( &(wdata->send_list), (struct WinNode *) mn );
        LeaveCriticalSection( &critical_section );

        return( data_size ); // muss zwar nicht stimmen, aber fuer Statistik
        }
    else
        return( 0L );
}


unsigned long wdp_SendMessage( struct windp_win_data *wdata, char *sender_id,
                               unsigned long sender_kind, char *receiver_id,
                               unsigned long receiver_kind, char *data,
                               unsigned long data_size, unsigned long guaranteed )
{
    /* -----------------------------------------------------------------
    ** Aus dem namen und der Art der Adresse muﬂ die DPID des Empf‰ngers
    ** ermittelt werden. Auﬂerdem ¸bergeben wir die DPID des Senders,
    ** der auch von auﬂen ¸bergeben werden muﬂ.
    ** ----------------------------------------------------------------*/
    HRESULT  hr;
    DPID     sender = 0, receiver = 0;
    unsigned long f = 0L;

    if( !(wdata->dpo2) ) return( 0L );

    /*** Daten zum Empf‰nger ermitteln ***/
    switch( receiver_kind ) {

        case MSG_UNKNOWN:

            /*** Das geht nicht ***/
            return( 0L );
            break;

        case MSG_PLAYER:

            if( receiver_id ) {

                receiver = wdp_PlayerIDFromName( wdata, receiver_id );
                if( receiver ) f = 1L;
                }

            if( !f ) return( 0L );
            break;

        case MSG_ALL:

            receiver = DPID_ALLPLAYERS;
            break;
        }

    /*** Daten zum Sender ermitteln ***/
    switch( sender_kind ) {

        case MSG_UNKNOWN:

            /*** Der Sender ist egal, Anonyme message ***/
            sender = 0;
            break;

        case MSG_PLAYER:

            if( sender_id ) {

                sender = wdp_PlayerIDFromName( wdata, sender_id );
                if( sender ) f = 1L;
                }

            if( !f ) return( 0L );
            break;

        case MSG_ALL:

            /*** Obwohl das Blˆdsinn ist... ***/
            sender = DPID_ALLPLAYERS;
            break;
        }

    /*** Und nun senden ***/
    switch( wdata->guaranteed_mode ) {

        case NWGM_Normal:

            /*** Wie bisher ***/
            if( guaranteed && (wdata->guaranteed) )
                hr = wdata->dpo2->lpVtbl->Send( wdata->dpo2, sender, receiver,
                                                DPSEND_GUARANTEED, (LPVOID) data,
                                                (DWORD) data_size );
            else
                hr = wdata->dpo2->lpVtbl->Send( wdata->dpo2, sender, receiver,
                                                0, (LPVOID) data,
                                                (DWORD) data_size );
            break;

        case NWGM_Guaranteed:

            /*** In jedem Falle guaranteed ***/
            hr = wdata->dpo2->lpVtbl->Send( wdata->dpo2, sender, receiver,
                                            DPSEND_GUARANTEED, (LPVOID) data,
                                            (DWORD) data_size );
            break;

        case NWGM_NotGuaranteed:

            /*** In jedem Falle NICHT guaranteed ***/
            hr = wdata->dpo2->lpVtbl->Send( wdata->dpo2, sender, receiver,
                                            0, (LPVOID) data,
                                            (DWORD) data_size );
            break;
        }

    if( hr != DP_OK ) {

        /* -------------------------------------------------------------
        ** Denn INVALIDPARAMS kommt, wenn  der Spieler bereits raus ist,
        ** uns aber die Message ueber sein Verlassen noch nicht erreicht
        ** hat. Ist halt Multitasking...
        ** -----------------------------------------------------------*/
        if( hr != DPERR_INVALIDPARAMS )
            wdp_ErrorMessage( hr,"SendMessage/Send" );
        return( 0L );
        }

    /*** Alles ok, wir geben die zahl der gesendeten Bytes zurueck ***/
    return( data_size );
}


unsigned long wdp_FlushBuffer( struct windp_win_data *wdata, char *sender_id,
                               unsigned long sender_kind, char *receiver_id,
                               unsigned long receiver_kind, unsigned long guaranteed )
{
    /* -----------------------------------------------------------------------
    ** Der Buffer soll geleert werden. Alle Sendedaten sind genauso, wie
    ** sie bei Sendmessage reinkommen, bis auf den Datenblock und seine Grˆﬂe.
    ** Deshalb geben wir Sender und Empf‰nger auch extern an, auch wenn es
    ** etwas nach Overkill aussieht.
    ** ---------------------------------------------------------------------*/
    unsigned long ret;

    if( !(wdata->dpo2) ) return( 0L );

    /*** Ist ¸berhaupt was da? ***/
    if( wdata->sendbufferoffset == 0 ) return( 0L );

    ret = wdp_AnnounceSendMessage( wdata, sender_id, sender_kind, receiver_id,
                                   receiver_kind, wdata->sendbuffer,
                                   wdata->sendbufferoffset, guaranteed );
                                   
    /*** Dem Thread das Senden signalisieren, also aufwecken ***/
    SetEvent( srthread.win_data->mevent );                               

    if( ret ) {

        /*** trotzdem alles r¸cksetzen ***/
        wdata->sendbufferoffset = 0;
        }

    /*** ret haelt anzahl gesendeter Bytes ***/
    return( ret );
}


unsigned long wdp_CollectMessage( struct windp_win_data *wdata, char *sender_id,
                                  unsigned long sender_kind, char *receiver_id,
                                  unsigned long receiver_kind, char *data,
                                  unsigned long data_size, unsigned long guaranteed )
{
    /* ---------------------------------------------------------------------
    ** Die reinkommende Message wird nicht sofort losgeschickt, sondern
    ** in den internen Messagebuffer geschrieben. Wenn dieser voll ist
    ** oder ein FLUSH erfolgt, werden die Messages losgeschickt. Diese
    ** m¸ssen den gleichen Adressaten haben. Deshalb muﬂ die Adresse dessen,
    ** f¸r den gesammelt wird, eingestellt sein. Dann kann schon im Aufruf
    ** in wpd_play unterschieden werden.
    ** Dieser besteht aus einer Folge von normalen Messages, die solange
    ** ausgelesen werden kˆnnen, bis der interne zeiger die Grˆﬂe des Daten-
    ** Blocks ¸berschreitet. So brauchen wir in diesem Block keine Extra-
    ** informationen zu halten.
    ** -------------------------------------------------------------------*/

    if( !(wdata->dpo2) ) return( 0L );

    if( wdata->sendbuffersize < wdata->sendbufferoffset + data_size ) {

        /* ----------------------------------------------------------------
        ** Der Buffer ist voll. Wir verschicken erstmal das, was wir haben.
        ** Danach wird alles zur¸ckgesetzt und die reingekommene message
        ** geschrieben, aber noch nicht gesendet.
        ** --------------------------------------------------------------*/
        wdp_FlushBuffer( wdata, sender_id, sender_kind, receiver_id,
                         receiver_kind, guaranteed );
        }

    /*** Message zum merken zu groﬂ ? ***/
    if( wdata->sendbuffersize < data_size ) {

        /* ---------------------------------------------------------------
        ** Buffer wurde vorher schon geflusht, weil die bedingung darueber
        ** ja auch erfuellt ist. Deshalb nur Senden anmelden.
        ** -------------------------------------------------------------*/

        /*** Sofort losschicken ***/
        return( wdp_AnnounceSendMessage( wdata, sender_id, sender_kind, receiver_id,
                                          receiver_kind, data, data_size, guaranteed ) );
        }

    /*** Message merken ***/
    memcpy( &(wdata->sendbuffer[ wdata->sendbufferoffset ]), data, data_size );
    wdata->sendbufferoffset += data_size;

    return( 1L );
}


unsigned long wdp_TryReceiveMessage( struct windp_win_data *wdata, char **data,
                                     char **sender_id, unsigned long *sender_kind,
                                     char **receiver_id,
                                     unsigned long *receiver_kind,
                                     unsigned long *message_kind,
                                     unsigned long *message_size)
{
    /* ----------------------------------------------------------------------
    ** neues "Thread-Frontend" fuer Receivemessage. Receivemessage wird nun
    ** nur noch durch den SRThread aufgerufen und merkt sich das Eintreffen
    ** einer Message.
    ** dazu erzeugt es eine Struktur mit den Daten und klinkt sie an das
    ** Ende einer Liste. Wir untersuchen den ersten Eintrag. Ist read gesetzt
    ** dann loeschen wir dies und geben den naechsten Eintrag zurueck, setzen
    ** bei diesem aber "read" was die struktur als bereits gemeldet markiert.
    ** --------------------------------------------------------------------*/
    int leer;

    EnterCriticalSection( &critical_section );
    leer = (int) IsListEmpty( &(wdata->recv_list));
    LeaveCriticalSection( &critical_section );

    if( leer ) {
        /*** nix da ***/
        return( 0L );
        }
    else {

        struct message_node *mn;

        EnterCriticalSection( &critical_section );
        mn = (struct message_node *) wdata->recv_list.lh_Head;

        /*** Wurde die Message schon gelesen? ***/
        if( mn->read ) {

            /*** Freigeben und naechste holen ***/
            mn = (struct message_node *) win_RemHead( &(wdata->recv_list));

            wdp_FreeMem( mn );

            /*** noch was da ? ***/
            if( IsListEmpty( &(wdata->recv_list)) ) {
                LeaveCriticalSection( &critical_section );
                return( 0L );
                }

            mn = (struct message_node *) wdata->recv_list.lh_Head;
            }
        LeaveCriticalSection( &critical_section );

        /*** Daten kopieren ***/
        *data          = &(mn->data);
        *sender_id     = mn->sender_id;
        *sender_kind   = mn->sender_kind;
        *receiver_id   = mn->receiver_id;
        *receiver_kind = mn->receiver_kind;
        *message_kind  = mn->message_kind;
        *message_size  = mn->recv_size;

        /*** als gelesen markieren ***/
        mn->read = 1L;

        return( mn->recv_size );
        }

}



unsigned long wdp_ReceiveMessage( struct windp_win_data *wdata, char **data,
                                  char **sender_id, unsigned long *sender_kind,
                                  char **receiver_id,
                                  unsigned long *receiver_kind,
                                  unsigned long *message_kind,
                                  unsigned long *message_size)
{
    /* ----------------------------------------------------------------
    ** Empf‰ngt eine Message und gibt Daten dar¸ber zur¸ck. Weil einige
    ** Messages Subklassenspezifisch sein kˆnnen, also von der Art der
    ** Netztreiber abh‰ngen, muﬂ ich diese gleich hier bearbeiten. Das
    ** sind bei DP die Systemmessages. Wenn ich die aber lese, brauche
    ** ich Speicher daf¸r. Somit verwalte den Speicher gleich hier.
    ** Der, welcher die Routine aufruft, darf den Datenblock auslesen,
    ** aber nicht freigeben.
    **
    ** Wenn etwas gelesen werden konnte, was f¸r die Allgemeinheit
    ** interessant ist, kommt TRUE zur¸ck.
    ** --------------------------------------------------------------*/
    DPID     sender, receiver;
    DWORD    size;
    LPVOID   block;
    HRESULT  hr;

    if( !(wdata->dpo2) ) return( 0L );

    /*** Default haben wir nix ***/
    *data         = NULL;
    *message_size = 0;

    /*** Zuerst versuchen, mit dem Standard-datenblock zu lesen ***/
    size = wdata->normalsize;
    memset( wdata->normalblock, 0, size );
    hr = wdata->dpo2->lpVtbl->Receive( wdata->dpo2, &sender, &receiver,
                                       DPRECEIVE_ALL, wdata->normalblock,
                                       &size);

    /*** Is was reingekommen **/
    if( hr == DPERR_NOMESSAGES ) {

        *message_size = 0;
        return( 0L );
        }

    if( hr != DP_OK ) {

        /*** War der Buffer zu klein ? ***/
        if( hr == DPERR_BUFFERTOOSMALL ) {

            /*** Alten Buffer freigeben und neuen allozieren ***/
            if( wdata->bigblock ) free( wdata->bigblock );
            wdata->bigsize = size;
            if( wdata->bigblock = malloc( wdata->bigsize ) ) {

                /*** Erneut versuchen ***/
                hr = wdata->dpo2->lpVtbl->Receive( wdata->dpo2, &sender,
                                                   &receiver, DPRECEIVE_ALL,
                                                   wdata->bigblock, &size);
                if( hr != DP_OK ) {

                    /*** Itze weeﬂ ich och nich mehr ***/
                    wdp_ErrorMessage( hr,"ReceiveMessage/Receive big" );
                    *message_size = 0;
                    return( 0L );
                    }
                else block = wdata->bigblock;
                }
            }
        else {

            /*** War was anderes. Das'n Fehler ***/
            wdp_ErrorMessage( hr, "ReceiveMessage/Receive normal" );
            *message_size = 0;
            return( 0L );
            }
        }
    else block = wdata->normalblock;

    /*** Ist es zuf‰llig eine Message, die wir auswerten m¸ssen? ***/
    if( DPID_SYSMSG == sender ) {

        /*** Da m¸ssn mir uns selber k¸mmern ***/
        DPMSG_GENERIC *msg;
        static char bl[ WDP_NORMALBLOCKSIZE ]; // wegen Stack-Fehler global

        msg = (DPMSG_GENERIC *) block;
        switch( msg->dwType ) {

            case DPSYS_CREATEPLAYERORGROUP:

                /*** Ein neuer externer Spieler ***/
                if( ((DPMSG_CREATEPLAYERORGROUP *)msg)->dwPlayerType ==
                    DPPLAYERTYPE_PLAYER ) {

                    wdp_CreatePlayer( wdata,
                        ((DPMSG_CREATEPLAYERORGROUP *)msg)->dpnName.lpszShortNameA,
                        0,
                        ((DPMSG_CREATEPLAYERORGROUP *)msg)->dpId,
                        0L );

                    /*** Umschichten ***/
                    *message_kind = MK_CREATEPLAYER;
                    strcpy( bl, ((DPMSG_CREATEPLAYERORGROUP *)msg)->dpnName.lpszShortNameA);
                    strcpy( wdata->normalblock, bl );
                    *data = wdata->normalblock;
                    }
                else
                    *message_kind = MK_SYSUNKNOWN;

                break;

            case DPSYS_DESTROYPLAYERORGROUP:

                /* -----------------------------------------------------------
                ** Ein Spieler wird gekillt Und wenn ich den Namen zur¸ckgeben
                ** will, so sollte ich mir den kopieren, B E V O R ich den
                ** Spieler freigebe! So eine Scheiﬂe!
                ** ---------------------------------------------------------*/
                if( ((DPMSG_DESTROYPLAYERORGROUP *)msg)->dwPlayerType ==
                    DPPLAYERTYPE_PLAYER ) {

                    char *dname;

                    if( dname = wdp_PlayerNameFromID( wdata,
                               ((DPMSG_DESTROYPLAYERORGROUP *)msg)->dpId ) ) {

                        /*** Name merken ***/
                        strcpy( wdata->normalblock, dname );

                        wdp_DestroyPlayer( wdata, dname );

                        /*** Umschichten ***/
                        *data = wdata->normalblock;
                        }
                    *message_kind = MK_DESTROYPLAYER;
                    }
                else
                    *message_kind = MK_SYSUNKNOWN; // weil keine daten

                break;

            case DPSYS_HOST:

                *message_kind = MK_HOST;
                break;

            case DPSYS_SETSESSIONDESC:

                /* ---------------------------------------------------------
                ** Nur Informationsmessage! Auf keinen Fall SETSESSIONDESC
                ** machen, weil ein Setzen wieder eine Message zur Folge hat
                ** und so weiter...
                ** -------------------------------------------------------*/
                *message_kind = MK_SETSESSIONDESC;
                
                break;

            default:

                /* --------------------------------------------------------------
                ** Eine unbekannte System-Message. Damit sie nicht falsch
                ** interpretiert wird, sie zumindest als eine solche kennzeichnen
                ** ------------------------------------------------------------*/
                *message_kind = MK_SYSUNKNOWN;
                break;
            }

        /* -------------------------------------------------------------
        ** wir geben aber 0 zur¸ck, weil es nicht f¸r die ÷ffentlichkeit
        ** bestimmt war.
        ** -----------------------------------------------------------*/
        *message_size = size;
        return( 1L );
        }

    /*** Ausf¸llen der R¸ckgabewerte ***/
    *message_kind = MK_NORMAL;

    /*** F¸r wen ist das? Suchen der ID ***/
    if( receiver == DPID_ALLPLAYERS ) {

        *receiver_kind = MSG_ALL;
        }
    else {

        /*** Ein Player? ***/
        if( !(*receiver_id = wdp_PlayerNameFromID( wdata, receiver) ) ) {

            *receiver_kind = MSG_UNKNOWN;
            }
        else {
            *receiver_kind = MSG_PLAYER;
            }
        }

    /*** Von wem kam den das? ***/

    /*** Ein Player? ***/
    if( !(*sender_id = wdp_PlayerNameFromID( wdata, sender) ) ) {

        *sender_kind = MSG_UNKNOWN;
        }
    else {
        *sender_kind = MSG_PLAYER;
        }
    

    *data         = block;
    *message_size = size;

    return( 1L );
}


void wdp_SetGuaranteedMode( struct windp_win_data *wdata, unsigned long data )
{
    /* --------------------------------------------------------------
    ** Setzt den Modus f¸r das Verschicken von Messages:
    ** NWGM_Normal  ........was die SendRoutine sagt, so soll es sein
    **                      (wie bisher)
    ** NWGM_Guaranteed......immer garantiert
    ** NWGM_NotGuaranteed...immer NICHT garantiert
    ** ------------------------------------------------------------*/
    wdata->guaranteed_mode = data;
}


void wdp_SetVersionCheck( struct windp_win_data *wdata, BOOL data )
{
    /* ---------------------------------------------------------------
    ** Schaltet den Versionscheck fuer Sessionnamen ein. Jede Session, 
    ** die nicht den speziellen String entaelt, fliegt raus.
    ** -------------------------------------------------------------*/
    wdata->versioncheck = data;
}


void wdp_SetDebug( struct windp_win_data *wdata, BOOL data )
{
    /* ---------------------------------------------------------------
    ** Schaltet den Versionscheck fuer Sessionnamen ein. Jede Session, 
    ** die nicht den speziellen String entaelt, fliegt raus.
    ** -------------------------------------------------------------*/
    wdata->debug = data;
}


void wdp_SetVersionIdent( struct windp_win_data *wdata, char *version )
{
    /*** Setzt dann genau diesen Identifizierungsstring ***/
    strncpy( wdata->versionident, version, STANDARD_NAMELEN * 2);
}    


void wdp_Diagnosis( struct windp_win_data *wdata, unsigned long *num_send,
                    unsigned long *num_recv, unsigned long *max_send,
                    unsigned long *max_recv, unsigned long *tme_send,
                    unsigned long *max_tme_send, unsigned long *num_bug_send )
{
    *num_send =     srthread.num_send;
    *num_recv =     srthread.num_recv;
    *max_send =     srthread.max_send;
    *max_recv =     srthread.max_recv;
    *tme_send =     srthread.tme_send;
    *max_tme_send = srthread.max_tme_send;
    *num_bug_send = srthread.num_bug_send;     
}                    

/* -----------------------------------------------------------------------
**                              K R E M P E L
** ---------------------------------------------------------------------*/

unsigned long wdp_GetCapsInfo( struct windp_win_data *wdata, unsigned long *maxbuffersize,
                               unsigned long *baud)
{
    DPCAPS  caps;
    HRESULT hr;

    /*** geht das ¸berhaupt? ***/
    if( !(wdata->dpo2) ) return( 0L );

    /*** Ist ¸berhaupt ne Session dazu eingestellt? ***/
    if( !( wdata->session_joined || wdata->session_created) ) return( 0L );

    ZeroMemory( &caps, sizeof( DPCAPS ) );
    caps.dwSize = sizeof( DPCAPS );

    hr = wdata->dpo2->lpVtbl->GetCaps( wdata->dpo2, &caps, 0 );

    if( hr != DP_OK ) {

        wdp_ErrorMessage( hr, "GetCapsInfo/GetCaps" );
        return( 0L );
        }

    *maxbuffersize = caps.dwMaxBufferSize;
    *baud          = (caps.dwHundredBaud * 100);

    return( 1L );
}


unsigned long wdp_LockSession( struct windp_win_data *wdata,
                               unsigned long block)
{
    HRESULT hr;
    LPDPSESSIONDESC2 desc;
    DWORD   size;

    /*** Die Sache geht wahrscheinlich nicht! ***/

    /*** geht das ¸berhaupt? ***/
    if( !(wdata->dpo2) ) return( 0L );

    /*** Ist ¸berhaupt ne Session dazu eingestellt? ***/
    if( !( wdata->session_joined || wdata->session_created) ) return( 0L );

    /*** Grˆﬂe holen, die grˆﬂer als DESC2 ist. Komisch... ***/
    hr = wdata->dpo2->lpVtbl->GetSessionDesc( wdata->dpo2, NULL, &size );
    if( (hr != DP_OK) && (hr != DPERR_BUFFERTOOSMALL) ) {

        wdp_ErrorMessage( hr, "LockSession/GetSessionDesc ask size" );
        return( 0L );
        }

    /*** Speicher holen. Passiert ja recht selten ***/
    desc = malloc( size );

    /*** Sessiondesc holen ***/
    hr = wdata->dpo2->lpVtbl->GetSessionDesc( wdata->dpo2, desc, &size );
    if( hr != DP_OK ) {

        wdp_ErrorMessage( hr,"LockSession/GetSessionDesc get msg" );
        free( desc );
        return( 0L );
        }

    if( block )
        desc->dwFlags |=  DPSESSION_JOINDISABLED;
    else
        desc->dwFlags &= ~DPSESSION_JOINDISABLED;

    hr = wdata->dpo2->lpVtbl->SetSessionDesc( wdata->dpo2, desc, 0 );
    if( hr != DP_OK ) {

        wdp_ErrorMessage( hr, "LockSession/SetSessionDesc" );
        free( desc );
        return( 0L );
        }

    free( desc );
    return( 1L );
}


void wdp_Reset( struct windp_win_data *wdata )
{
    /* -------------------------------------------------------------------
    ** Setzt das Object fpr ein neues Spiel zur¸ck. Hat nix mit Player und
    ** Sessions zu tun! Macht die Sachen, die beim erneuten Aufruf eines
    ** Levels/einer Session schiefgehen kˆnnen.
    ** -----------------------------------------------------------------*/
    
    /* -------------------------------------------------------------
    ** Fuer Modem, aber doch besser fuer alle mal Objekt freigeben
    ** und neu erzeugen, um alle Verbidungen zu schliessen. Darf nur 
    ** aufgerufen werden, wenn in keiner Session!
    ** -----------------------------------------------------------*/
    if( !(wdata->session_created || wdata->session_joined )) {
    
        HRESULT hr;
    
        if( wdata->dpo2 ) wdata->dpo2->lpVtbl->Release(wdata->dpo2);
        hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_INPROC_SERVER, 
                               &IID_IDirectPlay3A, &(wdata->dpo2) );
        if( hr != DP_OK ) {
    
            wdp_ErrorMessage( hr,"Reset/CoCreateInstance" );
            }
        }

    wdata->sendbufferoffset = 0;
}


unsigned long wdp_GetNumPlayers( struct windp_win_data *wdata )
{
    /* ------------------------------------------------------------
    ** WinBox und Attribute ist echt scheiﬂe... Deshalb mit Methode
    ** ----------------------------------------------------------*/

    return( wdata->num_players );
}


/* -----------------------------------------------------------------------
**              F E R N S T E U E R U N G / L O B B Y
** ---------------------------------------------------------------------*/

unsigned long wdp_CheckRemoteStart( struct windp_win_data *wdata, char *name,
                                    unsigned char *is_host,
                                    unsigned char *remote )
{
    /* -------------------------------------------------------------------
    ** Liefert TRUE, wenn eine Aussage getroffen werden kann. Ist dem so,
    ** entscheidet remote ¸ber die G¸ltigkeit der Aussagen in der Struktur
    ** -----------------------------------------------------------------*/
    LPDIRECTPLAYLOBBYA  dpl1;
    LPDIRECTPLAYLOBBY2A dpl2;
    LPDIRECTPLAY2A      dpo3;
    HRESULT             hr;
    DWORD               size;
    LPDPLCONNECTION     conn = NULL;

    /*** Lobby-Objekt erzeugen ***/
    hr = DirectPlayLobbyCreate( NULL, &dpl1, NULL, NULL, 0);

    /*** Fehler --> dann kein externer Start ***/
    if( DP_OK != hr ) {

        wdp_ErrorMessage( hr, "CheckRemoteStart/DirectPlayLobbyCreate" );
        return( FALSE );
        }

    /*** ANSI-DPL2-Interface erzeugen ***/
    hr = dpl1->lpVtbl->QueryInterface( dpl1, &(IID_IDirectPlayLobby2A),
                                             (LPVOID *) &(dpl2));
    if( DP_OK != hr ) {

        dpl1->lpVtbl->Release( dpl1 );
        wdp_ErrorMessage( hr,"CheckRemoteStart/QueryInterface 1" );
        return( FALSE );
        }

    /*** das alte kann freigegeben werden ***/
    dpl1->lpVtbl->Release( dpl1 );
    
    /*** Grˆﬂe der Struktur abfragen ***/
    hr = dpl2->lpVtbl->GetConnectionSettings( dpl2, 0, NULL, &size );
    if( DPERR_BUFFERTOOSMALL == hr ) {

        /*** Speicher bereitstellen ***/
        if( conn = (LPDPLCONNECTION) malloc( size ) ) {

            /*** Verbindung abfragen ***/
            hr = dpl2->lpVtbl->GetConnectionSettings( dpl2, 0, conn, &size );
            }
        else {

            dpl2->lpVtbl->Release( dpl2 );
            return( FALSE );
            }
        }

    /* -----------------------------------------------------
    ** nicht lobbied? raus!
    ** Dieser R¸ckkehrwert kommt auch bei der Grˆﬂenabfrage.
    ** ---------------------------------------------------*/
    if( DPERR_NOTLOBBIED == hr ) {

        if( conn ) free( conn );
        dpl2->lpVtbl->Release( dpl2 );
        *remote = 0;
        return( TRUE );
        }

    /*** anderweitiger Fehler ***/
    if( conn && (DP_OK != hr) ) {

        wdp_ErrorMessage( hr, "CheckRemoteStart/GetConnectionSettings" );
        free( conn );
        dpl2->lpVtbl->Release( dpl2 );
        return( FALSE );
        }

    /*** Lobbied und kein Fehler, also auswerten ***/
    *remote = 1;

    /*** Host ***/
    if( conn->dwFlags & DPLCONNECTION_CREATESESSION ) {
        wdata->session_created = 1;
        *is_host = 1;
        }
    else {
        wdata->session_joined = 1;
        *is_host = 0;
        }

    /* ------------------------------------------------------------------
    ** Name, muﬂ da sein! Und muss auch in jedem Falle zurueckgegeben
    ** werden, weil sonst der gemerkte Netzname als Sender genommen wird,
    ** was natuerlich Fehler ergibt, wenn der LobbyName anders ist.
    ** ----------------------------------------------------------------*/
    memset(  name, 0, STANDARD_NAMELEN );
    if( conn->lpPlayerName->lpszShortNameA )
        strncpy( name, conn->lpPlayerName->lpszShortNameA, STANDARD_NAMELEN-1 );
    else
        strcpy( name, "PLAYER" );

    /*** Sessionflags nachkorrigieren ***/
    conn->lpSessionDesc->dwFlags      = DPSESSION_MIGRATEHOST | DPSESSION_KEEPALIVE;
    conn->lpSessionDesc->dwMaxPlayers = MAXNUM_PLAYERS;
    hr = dpl2->lpVtbl->SetConnectionSettings( dpl2, 0, 0, conn );
    if( hr != DP_OK ) {

        wdp_ErrorMessage( hr,"CheckremoteStart/SetConnectionSettings" );
        free( conn );
        dpl2->lpVtbl->Release( dpl2 );
        return( FALSE );
        }

    /*** Aufr‰umen evtl. bestehender Einstellungen ***/
    if( wdata->dpo2 ) wdata->dpo2->lpVtbl->Release( wdata->dpo2 );
    if( wdata->dpo1 ) wdata->dpo1->lpVtbl->Release( wdata->dpo1 );
    wdata->dpo1 = NULL;

    /*** Verbinden ***/
    hr = dpl2->lpVtbl->Connect( dpl2, 0, &(dpo3), NULL );
    if( DP_OK != hr ) {

        wdp_ErrorMessage( hr, "CheckRemoteStart/Connect" );
        free( conn );
        dpl2->lpVtbl->Release( dpl2 );
        return( FALSE );
        }
        
    hr = dpo3->lpVtbl->QueryInterface( dpo3, &(IID_IDirectPlay3A), (LPVOID *) &(wdata->dpo2) );
    if( DP_OK != hr ) {

        wdp_ErrorMessage( hr, "CheckRemoteStart/QueryInterface" );
        free( conn );
        dpl2->lpVtbl->Release( dpl2 );
        dpo3->lpVtbl->Release( dpo3 );
        return( FALSE );
        }

    /*** Sessionname ausf¸llen und act_session auf 0 stellen ***/
    wdata->act_session = 0;
    if( conn->dwFlags & DPLCONNECTION_CREATESESSION ) {

        wdata->session_created = TRUE;
        wdata->session_joined  = FALSE;
        if( conn->lpSessionDesc->lpszSessionNameA )
            strcpy( wdata->own_session.name,
                    conn->lpSessionDesc->lpszSessionNameA );
        else
            wdata->own_session.name[0] = 0;           
        }
    else {

        wdata->session_created = FALSE;
        wdata->session_joined  = TRUE;
        if( conn->lpSessionDesc->lpszSessionNameA )
            strcpy( wdata->session[ wdata->act_session ].name,
                    conn->lpSessionDesc->lpszSessionNameA );
        else
            wdata->session[ wdata->act_session ].name[0] = 0;           
        }

    /*** Bisherige Player ermitteln ***/
    wdp_EnumPlayers( wdata );

    /*** Player erzeugen, DPID wird doch bei CREATE ausgef¸llt ***/
    if( !wdp_CreatePlayer( wdata, name, WDPF_OWNPLAYER, 0, 0L ) ) {

        free( conn );
        dpl2->lpVtbl->Release( dpl2 );
        dpo3->lpVtbl->Release( dpo3 );
        return( FALSE );
        }

    /*** Aufr‰umen ***/
    free( conn );
    dpo3->lpVtbl->Release( dpo3 );
    dpl2->lpVtbl->Release( dpl2 );
    return( TRUE );

}


/* -----------------------------------------------------------------------
**                                 T H R E A D S
** ---------------------------------------------------------------------*/
unsigned long wdp_SRThread( LPVOID t )
{
    /*** Argumentuebergabe klappt irgendwie nicht... ***/
    struct WinNode *n;
    DWORD  event;
    unsigned long num_send, num_recv;

    /***warten auf Ereignis oder Timeout ***/
    while( 1 ) {

        struct message_node *mns;
        
        event = WaitForSingleObject( srthread.win_data->mevent, INFINITE );

        /*** Senden angefordert? Sendebuffer leerraeumen ***/
        num_send = 0;
        num_recv = 0;
        while( 1 ) {

            /*** Listenzugriff absichern ***/
            EnterCriticalSection( &critical_section );
            mns = (struct message_node *)
                   win_RemHead( &(srthread.win_data->send_list) );
            LeaveCriticalSection( &critical_section );

            if( mns ) {
            
                unsigned long m;

                /*** Einen richtigen Sendeaufruf ***/
                //EnterCriticalSection( &critical_section );
                m = yw_StartProfile();
                
                if( !wdp_SendMessage( srthread.win_data,
                                       mns->sender_id, mns->sender_kind,
                                       mns->receiver_id, mns->receiver_kind,
                                       &(mns->data), mns->recv_size,
                                       srthread.guaranteed ))
                    srthread.num_bug_send++;
                                 
                srthread.tme_send = yw_EndProfile( m );
                if( srthread.tme_send > srthread.max_tme_send )
                    srthread.max_tme_send = srthread.tme_send;
                //LeaveCriticalSection( &critical_section );
                
                num_send++;

                /*** erledigt. freigeben ***/
                wdp_FreeMem( mns );
                }
            else
                break;
            }
            
        /* ----------------------------------------------------------
        ** fuer die Statistik. Weil der Thread meistens nichts macht,
        ** die 0 ignorieren 
        ** --------------------------------------------------------*/
        srthread.num_send = num_send;    
        if( srthread.num_send > srthread.max_send )
            srthread.max_send = srthread.num_send;

        /* ----------------------------------------------------------------
        ** Daten reingekommen? Dann vom Port nehmen. Um receivemessage
        ** moeglichst nicht zu veraendern, erzeuge ich die Nodes erst
        ** hier. Da wird zwar doppelt kopiert, aber es bleibt abgeschottet.
        ** --------------------------------------------------------------*/
        if( WAIT_OBJECT_0 == event ) {

            while( wdp_ReceiveMessage( srthread.win_data, &srthread.recv_data,
                                       &srthread.r_sender_id, &srthread.r_sender_kind,
                                       &srthread.r_receiver_id, &srthread.r_receiver_kind,
                                       &srthread.message_kind, &srthread.recv_size)) {

                /* --------------------------------------------------------
                ** recv_data zeigt auf  den ausgefuellten normalblock. Wenn
                ** jemand versucht, die Message zu lesen, kopieren wir sie
                ** in unseren eigenen Buffer, so dass recv_data erneut auf
                ** normalblock gebogen werden kann. Dieser Buffer ist eine
                ** Node, die ich immer alloziere.
                ** ------------------------------------------------------*/
                struct message_node *mnr;
                int node_size;
                
                num_recv++;

                node_size = sizeof( struct message_node ) + srthread.recv_size;
                node_size += 32;     // nur zur Sicherheit
                mnr = wdp_AllocMem( node_size );
                if( mnr ) {

                    /*** Ausfuellen ***/
                    memset( mnr, 0, node_size );
                    if( srthread.r_sender_id )
                        strcpy( mnr->sender_id, srthread.r_sender_id );
                    if( srthread.r_receiver_id )
                        strcpy( mnr->receiver_id, srthread.r_receiver_id );
                    mnr->sender_kind   = srthread.r_sender_kind;
                    mnr->receiver_kind = srthread.r_receiver_kind;
                    mnr->message_kind  = srthread.message_kind;
                    mnr->recv_size     = srthread.recv_size;
                    if( srthread.recv_data )
                        memcpy( &(mnr->data), srthread.recv_data, srthread.recv_size );

                    /*** Und einklinken. Zugriffsrechte holen ***/
                    EnterCriticalSection( &critical_section );
                    win_AddTail( &(srthread.win_data->recv_list), (struct WinNode *)mnr );
                    LeaveCriticalSection( &critical_section );
                    }
                }
            }
            
        /*** Statistik-Arbeit ***/
        srthread.num_recv = num_recv; 
        if( num_recv )           
        if( srthread.num_recv > srthread.max_recv )
            srthread.max_recv = srthread.num_recv;

        /*** Verlassen angefordert? ***/
        if( srthread.quit )
            break;
        }

    /*** buffer freigeben ***/
    free( srthread.send_buffer );
    free( srthread.recv_buffer );

    /*** Empfansliste aufraeumen ***/
    EnterCriticalSection( &critical_section );
    while( n = win_RemHead( &(srthread.win_data->recv_list)) )
        wdp_FreeMem( n );

    while( n = win_RemHead( &(srthread.win_data->send_list)) )
        wdp_FreeMem( n );

    LeaveCriticalSection( &critical_section );

    /*** Als beendet markieren ***/
    srthread.quit = 0;

    ExitThread(0);
}


/* -----------------------------------------------------------------------
**                       S E R V I C E R O U T I N E N
** ---------------------------------------------------------------------*/

char *wdp_PlayerNameFromID( struct windp_win_data *wdata, DPID id)
{
    unsigned long i = wdata->num_players;
    while( i ) {

        if( wdata->player[ i - 1 ].dpid == id )
            return( wdata->player[ i - 1 ].name );
        i--;
        }

    return( NULL );
}


unsigned long wdp_PlayerIDFromName( struct windp_win_data *wdata, char *name )
{
    unsigned long i = wdata->num_players;
    while( i ) {

        if( stricmp( wdata->player[ i - 1 ].name, name) == 0 )
            return( wdata->player[ i - 1 ].dpid );
        i--;
        }

    return( NULL );
}


unsigned long wdp_PlayerOffsetFromName( struct windp_win_data *wdata, char *name )
{
    unsigned long i = wdata->num_players;
    while( i ) {

        if( stricmp( wdata->player[ i - 1 ].name, name) == 0 )
            return( i - 1 );
        i--;
        }

    return( -1 );
}

//---------------------- SPEICHER ----------------------------------------

LPVOID wdp_AllocMem( unsigned long size )
{
    return( (LPVOID) GlobalAlloc( GMEM_FIXED, size ));
}

void wdp_FreeMem( void *mem )
{
    GlobalFree( mem );
}


#endif // von __NETWORK__
