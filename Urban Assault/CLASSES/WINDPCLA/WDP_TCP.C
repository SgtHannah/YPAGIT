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

BOOL wdp_AskLocalMachine( void *wdata, char *address, char *name, long namelen, 
                          char *address_string, long address_string_len );

/*-----------------------------------------------------------------
**                      P R O V I D E R
**---------------------------------------------------------------*/

_dispatcher( BOOL, wdp_NWM_ASKLOCALMACHINE, struct localmachine_msg *loc )
{
    /* ----------------------------------------------------------------
    ** Providerabfrage. Die Deklaration des Hooks und die
    ** entsprechenden Routinen sind Windows-Spezifisch und laufen somit
    ** in der Box. Dies ist also nur die Schale für VFM
    ** --------------------------------------------------------------*/
    struct windp_data *wdp;

    wdp = INST_DATA(cl, o );

    return( wdp_AskLocalMachine( (void *)(wdp->win_data), loc->address, loc->name, STANDARD_NAMELEN,
                                  loc->address_string, STANDARD_NAMELEN ) );
}

#endif