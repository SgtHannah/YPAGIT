/*
**  $Source: $
**  $Revision: $
**  $Date: $
**  $Locker: $
**  $Author: $
**
*/
#define WINDP_WINBOX            // itze wirds Box-Zeuch
#define INITGUID

#include "nucleus/sys.h"
#include "network/windpclass.h"  // Public Version
#include "network/winlist.h"

#ifdef __NETWORK__

/*-----------------------------------------------------------------*/

/*** Brot-O-Typen ***/
unsigned long wdp_InitWinStuff( struct windp_data *wdp);
LPVOID        wdp_FreeWinStuff( struct windp_data *wdp);
extern CRITICAL_SECTION critical_section;


/* ----------------------------------------------------------------
**                      Suport-Routinen
** --------------------------------------------------------------*/
unsigned long wdp_InitWinStuff( struct windp_data *wdp)
{
    HRESULT hr;
    
    if( sizeof( struct windp_win_data ) > 0 ) {

        /*** Es lohnt zu allozieren und der Wert hat ok zu sein ***/
        if( !(wdp->win_data = (struct windp_win_data *)
             malloc( sizeof( struct windp_win_data )) ) )
             return( 0L );
        }
    memset( wdp->win_data, 0, sizeof( struct windp_win_data ) );

    /*** Empfangsbuffer anlegen ***/
    if( wdp->win_data->normalblock = malloc( WDP_RECEIVEBUFFERSIZE ) )
        wdp->win_data->normalsize  = WDP_RECEIVEBUFFERSIZE;
    else {

        wdp_FreeWinStuff( wdp );
        return( 0L );
        }

    /*** Empfangslisten ***/
    win_NewList( &(wdp->win_data->recv_list) );
    win_NewList( &(wdp->win_data->send_list) );
    InitializeCriticalSection( &critical_section );

    /*** Sendebuffer anlegen ***/
    if( wdp->win_data->sendbuffer      = malloc( WDP_NORMALBLOCKSIZE ) )
        wdp->win_data->sendbuffersize  = WDP_NORMALBLOCKSIZE;
    else {

        wdp_FreeWinStuff( wdp );
        return( 0L );
        }
        
    /*** so, itze ersct emol COM vorbereiten ***/
    if( FAILED( CoInitialize( NULL ) ) ) {
    
        /*** ohne COM kein Objekt und damit kein DPlay ***/
        wdp_FreeWinStuff( wdp );
        return( 0L );
        }

    /* -----------------------------------------------------------------------------------------------
    ** Erzeugen des DP3-Objektes. Die Provider/Connections werden analog den Sessions gesetzt, brauche
    ** ich also nicht hier zu setzen.
    ** ---------------------------------------------------------------------------------------------*/
    wdp->win_data->dpo1 = NULL; // wird nicht laenger benoetigt
    wdp->win_data->dpo2 = NULL; // sollte trotzdem geloescht sein
    hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlay3A, &(wdp->win_data->dpo2) );
    if( hr != DP_OK ) {
    
        wdp_FreeWinStuff( wdp );
        return( 0L );
        }
    
    /*** Und Tschüß ***/
    return( 1L );
}

LPVOID wdp_FreeWinStuff( struct windp_data *wdp )
{
    struct windp_win_data *wdata;

    /*** War was alloziert? ***/
    if( wdp->win_data ) {
    
        int i;

        wdata = wdp->win_data;

        DeleteCriticalSection( &critical_section );

        /*** Provider abmelden ***/
        if( wdata->dpo2 ) wdata->dpo2->lpVtbl->Release(wdata->dpo2);
        if( wdata->dpo1 ) wdata->dpo1->lpVtbl->Release(wdata->dpo1);

        /*** Empfangsbuffer umlegen ***/
        if( wdp->win_data->normalblock )
            free( wdp->win_data->normalblock );

        /*** Sendebuffer umlegen ***/
        if( wdp->win_data->sendbuffer )
            free( wdp->win_data->sendbuffer );

        /*** Connection-Speicher freigeben ***/    
        for( i = 0; i < wdata->num_providers; i++ )
            if( wdata->provider[ i ].connection ) free( wdata->provider[ i ].connection );
            
        free( wdp->win_data );
        wdp->win_data = NULL;
        }
        
    /*** COM-faehigkeit wieder abmelden ***/
    CoUninitialize();    

    return;
}

#endif // von __NETWORK__
