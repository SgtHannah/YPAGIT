/*
**  $Source: $
**  $Revision: $
**  $Date: $
**  $Locker: $
**  $Author: $
**
*/

#define WINDP_WINBOX            // itze wirds Box-Zeuch

#include "nucleus/sys.h"
#include "network/windpclass.h"  // Public Version
#include "winsock.h"

#ifdef __NETWORK__

/*** definiert in win_main.c ***/
extern HWND win_HWnd;

unsigned long wdp_AskLocalMachine( struct windp_win_data *wdata, char *address, char *name, long namelen,
                                   char *address_string, long address_string_len )
{
    /* ----------------------------------------------------------------- 
    ** Versucht die Adresse zu erfragen. War dies moeglich, so wird TRUE
    ** zurueckgegeben. address ist ein 4-byte-Array. 
    ** ---------------------------------------------------------------*/
    WORD    version;
    WSADATA wsadata;
    
    version = MAKEWORD( 1, 1 );
    if( !WSAStartup( version, &wsadata ) ) {
    
        BOOL address_found = FALSE;
        char FAR hostname[ 255 ];
    
        /*** Name des "Heim"-Computers ***/
        if( !gethostname( hostname, 255 ) ) {
                
            /*** zum namen die Adresse erfragen ***/
            struct hostent *host;
            
            if( host = gethostbyname( hostname ) ) {
            
                /*** Adresse und Name merken ***/
                memcpy( address, host->h_addr_list[ 0 ], 4);
                strncpy( name, hostname, namelen );
                strncpy( address_string, 
                         inet_ntoa( *((struct in_addr *)address) ), 
                         address_string_len );
                address_found = TRUE;
                }
            }        
        
        if( address_found )
            return( TRUE );
        else
            return( FALSE );    
        }
        
    return( FALSE );
}

#endif
       
