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

#ifdef __NETWORK__

/*** definiert in win_main.c, ausgefuellt einmalig hier ***/
extern HWND win_HWnd;

unsigned long wdp_Log( char* string, ... );

LPVOID wdp_ErrorMessage( HRESULT hr, char *wo )
{
    char buf[1024];
    char *err;

    if (hr == DPERR_ALREADYINITIALIZED)       err = "Already Initialized";
    else if (hr == DPERR_ACCESSDENIED)        err = "Access Denied";
    else if (hr == DPERR_ACTIVEPLAYERS)       err = "Active Players";
    else if (hr == DPERR_BUFFERTOOSMALL)      err = "Buffer to Small";
    else if (hr == DPERR_CANTADDPLAYER)       err = "Can't add player";
    else if (hr == DPERR_CANTCREATEGROUP)     err = "Can't create group";
    else if (hr == DPERR_CANTCREATEPLAYER)    err = "Can't create Player";
    else if (hr == DPERR_CANTCREATESESSION)   err = "Cant't create session";
    else if (hr == DPERR_CAPSNOTAVAILABLEYET) err = "Caps not available yet";
    else if (hr == DPERR_EXCEPTION)           err = "Exception";
    else if (hr == DPERR_GENERIC)             err = "Generic";
    else if (hr == DPERR_INVALIDFLAGS)        err = "Invalid Flags";
    else if (hr == DPERR_INVALIDOBJECT)       err = "Invalid Object";
    else if (hr == DPERR_INVALIDPARAM)        err = "Invalid Parameter";
    else if (hr == DPERR_INVALIDPARAMS)       err = "Invalid Parameters";
    else if (hr == DPERR_INVALIDPLAYER)       err = "Invalid Players";
    else if (hr == DPERR_NOCAPS)              err = "No Caps";
    else if (hr == DPERR_NOCONNECTION)        err = "No Connection";
    else if (hr == DPERR_NOMEMORY)            err = "No memory";
    else if (hr == DPERR_OUTOFMEMORY)         err = "Out of memory";
    else if (hr == DPERR_NOMESSAGES)          err = "No messages";
    else if (hr == DPERR_NONAMESERVERFOUND)   err = "No Nameserver found";
    else if (hr == DPERR_NOPLAYERS)           err = "No Players";
    else if (hr == DPERR_NOSESSIONS)          err = "No Sessions";
    else if (hr == DPERR_SENDTOOBIG)          err = "send too big";
    else if (hr == DPERR_TIMEOUT)             err = "Time out";
    else if (hr == DPERR_UNAVAILABLE)         err = "unavailable";
    else if (hr == DPERR_UNSUPPORTED)         err = "Unsupported";
    else if (hr == DPERR_BUSY)                err = "Busy";
    else if (hr == DPERR_USERCANCEL)          err = "Usercancel";
    else if (hr == DPERR_NOINTERFACE)         err = "No Interface";
    else if (hr == DPERR_CANNOTCREATESERVER)  err = "Cannot create server";
    else if (hr == DPERR_PLAYERLOST)          err = "Player lost";
    else if (hr == DPERR_SESSIONLOST)         err = "Session lost";
    else if (hr == DPERR_BUFFERTOOLARGE)      err = "Buffer too large";
    else if (hr == DPERR_CANTCREATEPROCESS)   err = "Cant create process";
    else if (hr == DPERR_APPNOTSTARTED)       err = "App not started";
    else if (hr == DPERR_INVALIDINTERFACE)    err = "Invalid Interface";
    else if (hr == DPERR_NOSERVICEPROVIDER)   err = "No service provider";
    else if (hr == DPERR_UNKNOWNAPPLICATION)  err = "Unknown Application";
    else if (hr == DPERR_NOTLOBBIED)          err = "Not lobbied";
    else err = "Unknown error message";

    if( wo )
        sprintf( buf, "in %s: \n-----------\n %s (%x)\0", wo, err, hr );
    else    
        sprintf( buf, "%s (%x)\0", err, hr );

//    erst wieder, wenn screenflip-loesung      
//    if (win_HWnd)
//        MessageBox(win_HWnd,buf,"YPA: DPlay Error",0);
//    else
//        printf(" DPlay Error: %s\n", buf ); // LogMsg geht in der Box nich
    
    if( wo )
        wdp_Log( "Drastic YPA DPlay error: \n    %s (%d) in %s\n", err, hr, wo);
}


LPVOID wdp_StringMessage( char *text )
{
    if (win_HWnd)
        MessageBox(win_HWnd,text,"YPA: stringmessage",0);
    else
        printf(" YPA: stringmessage: %s\n", text ); // LogMsg geht in der Box nich
}


LPVOID wdp_NumberMessage( DWORD number )
{
    char text[ 300 ];
    sprintf( text, "--> : %ld", number );

    if (win_HWnd)
        MessageBox(win_HWnd,text,"YPA: numbermessage",0);
    else
        printf(" YPA: numbermessage: %s\n", text ); // LogMsg geht in der Box nich
}


#endif // von __NETWORK__
