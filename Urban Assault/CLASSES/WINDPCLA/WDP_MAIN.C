/*
**  $Source: $
**  $Revision: $
**  $Date: $
**  $Locker: $
**  $Author: $
**
*/
#include <exec/types.h>
#include <exec/memory.h>

#include "nucleus/nucleus2.h"
#include "network/windpclass.h"  // Public Version

#ifdef __NETWORK__

/*-----------------------------------------------------------------*/
_dispatcher(Object *, wdp_OM_NEW, struct TagItem *);
_dispatcher(ULONG, wdp_OM_DISPOSE, void *);
_dispatcher(void, wdp_OM_GET, struct TagItem *);

_dispatcher(void, wdp_NWM_ASKPROVIDERS, void *);
_dispatcher(BOOL, wdp_NWM_GETPROVIDERNAME, struct getprovidername_msg *);
_dispatcher(BOOL, wdp_NWM_SETPROVIDER, struct setprovider_msg *);
_dispatcher(BOOL, wdp_NWM_GETPROVIDER, struct getprovider_msg *);

_dispatcher(void, wdp_NWM_ASKSESSIONS, void *);
_dispatcher(BOOL, wdp_NWM_GETSESSIONNAME, struct getsessionname_msg *);
_dispatcher(BOOL, wdp_NWM_OPENSESSION, struct opensession_msg *);
_dispatcher(BOOL, wdp_NWM_JOINSESSION, struct joinsession_msg *);
_dispatcher(BOOL, wdp_NWM_CLOSESESSION, void *);
_dispatcher(BOOL, wdp_NWM_GETSESSION, struct getsession_msg *);
_dispatcher(ULONG, wdp_NWM_GETSESSIONSTATUS, void *);
_dispatcher(BOOL, wdp_NWM_SETSESSIONNAME, struct setsessionname_msg *);

_dispatcher(BOOL, wdp_NWM_CREATEPLAYER, struct createplayer_msg *);
_dispatcher(BOOL, wdp_NWM_DESTROYPLAYER, struct destroyplayer_msg *);
_dispatcher(BOOL, wdp_NWM_ENUMPLAYERS, void *);
_dispatcher(BOOL, wdp_NWM_GETPLAYERDATA, struct getplayerdata_msg *);

_dispatcher(BOOL, wdp_NWM_SENDMESSAGE, struct sendmessage_msg *);
_dispatcher(BOOL, wdp_NWM_RECEIVEMESSAGE, struct receivemessage_msg *);
_dispatcher(BOOL, wdp_NWM_FLUSHBUFFER, struct flushbuffer_msg *);

_dispatcher(BOOL, wdp_NWM_GETCAPSINFO,   struct getcapsinfo_msg *);
_dispatcher(BOOL, wdp_NWM_LOCKSESSION,   struct locksession_msg *);
_dispatcher(void, wdp_NWM_RESET,         void *);
_dispatcher(void, wdp_NWM_GETNUMPLAYERS, void *);

_dispatcher(BOOL, wdp_NWM_CHECKREMOTESTART, struct checkremotestart_msg *);

_dispatcher(BOOL, wdp_NWM_ASKLOCALMACHINE, struct localmachine_msg *loc );
_dispatcher(void, wdp_NWM_SETSESSIONIDENT, struct sessionident_msg *si );
_dispatcher(ULONG,wdp_NWM_GETPROVIDERTYPE, void *nix );

_dispatcher(void,wdp_NWM_DIAGNOSIS, struct diagnosis_msg *dm );

/*** Brot-O-Typen ***/
BOOL wdp_InitVFMStuff( struct windp_data *wdp);
void wdp_FreeVFMStuff( struct windp_data *wdp);

extern BOOL wdp_InitWinStuff( struct windp_data *wdp);
extern void wdp_FreeWinStuff( struct windp_data *wdp);
extern unsigned long wdp_GetNumPlayers( void *wdata );
extern unsigned long wdp_SetGuaranteedMode( void *wdata, unsigned long data );
extern unsigned long wdp_SetVersionCheck(   void *wdata, BOOL data );

#define WINDP_NUM_CONFIG_ITEMS (2)
struct ConfigItem wdp_ConfigItems[WINDP_NUM_CONFIG_ITEMS] = {
    {"net.gmode",        CONFIG_INTEGER, 0},
    {"net.versioncheck", CONFIG_BOOL,    TRUE},
};

/*-------------------------------------------------------------------
**  Class Header
*/
ULONG wdp_Methods[NUCLEUS_NUMMETHODS];
struct ClassInfo wdp_clinfo;

struct ClassInfo *wdp_MakeClass(ULONG id,...);
BOOL wdp_FreeClass(void);

struct GET_Class wdp_GET = {
    &wdp_MakeClass,
    &wdp_FreeClass,
};

struct GET_Class *wdp_Entry(void)
{
    return(&wdp_GET);
};

struct SegInfo windp_class_seg = {
    { NULL, NULL,
      0, 0,
      "MC2classes:windp.class"
    },
    wdp_Entry,
};

/*-----------------------------------------------------------------*/
struct ClassInfo *wdp_MakeClass(ULONG id,...)
/*
**  CHANGED
**      07-Nov-96   floh    created
*/
{
    /*** Klasse initialisieren ***/
    memset(wdp_Methods,0,sizeof(wdp_Methods));

    wdp_Methods[OM_NEW]      = (ULONG) wdp_OM_NEW;
    wdp_Methods[OM_DISPOSE]  = (ULONG) wdp_OM_DISPOSE;
    wdp_Methods[OM_GET]      = (ULONG) wdp_OM_GET;

    /*** Network Class ***/
    wdp_Methods[NWM_ASKPROVIDERS]       = (ULONG) wdp_NWM_ASKPROVIDERS;
    wdp_Methods[NWM_GETPROVIDERNAME]    = (ULONG) wdp_NWM_GETPROVIDERNAME;
    wdp_Methods[NWM_SETPROVIDER]        = (ULONG) wdp_NWM_SETPROVIDER;
    wdp_Methods[NWM_GETPROVIDER]        = (ULONG) wdp_NWM_GETPROVIDER;
    wdp_Methods[NWM_GETPROVIDERTYPE]    = (ULONG) wdp_NWM_GETPROVIDERTYPE;

    wdp_Methods[NWM_ASKSESSIONS]        = (ULONG) wdp_NWM_ASKSESSIONS;
    wdp_Methods[NWM_GETSESSIONNAME]     = (ULONG) wdp_NWM_GETSESSIONNAME;
    wdp_Methods[NWM_OPENSESSION]        = (ULONG) wdp_NWM_OPENSESSION;
    wdp_Methods[NWM_JOINSESSION]        = (ULONG) wdp_NWM_JOINSESSION;
    wdp_Methods[NWM_CLOSESESSION]       = (ULONG) wdp_NWM_CLOSESESSION;
    wdp_Methods[NWM_GETSESSION]         = (ULONG) wdp_NWM_GETSESSION;
    wdp_Methods[NWM_GETSESSIONSTATUS]   = (ULONG) wdp_NWM_GETSESSIONSTATUS;
    wdp_Methods[NWM_SETSESSIONNAME]     = (ULONG) wdp_NWM_SETSESSIONNAME;

    wdp_Methods[NWM_CREATEPLAYER]       = (ULONG) wdp_NWM_CREATEPLAYER;
    wdp_Methods[NWM_DESTROYPLAYER]      = (ULONG) wdp_NWM_DESTROYPLAYER;
    wdp_Methods[NWM_ENUMPLAYERS]        = (ULONG) wdp_NWM_ENUMPLAYERS;
    wdp_Methods[NWM_GETPLAYERDATA]      = (ULONG) wdp_NWM_GETPLAYERDATA;

    wdp_Methods[NWM_SENDMESSAGE]        = (ULONG) wdp_NWM_SENDMESSAGE;
    wdp_Methods[NWM_RECEIVEMESSAGE]     = (ULONG) wdp_NWM_RECEIVEMESSAGE;
    wdp_Methods[NWM_FLUSHBUFFER]        = (ULONG) wdp_NWM_FLUSHBUFFER;

    wdp_Methods[NWM_GETCAPSINFO]        = (ULONG) wdp_NWM_GETCAPSINFO;
    wdp_Methods[NWM_LOCKSESSION]        = (ULONG) wdp_NWM_LOCKSESSION;
    wdp_Methods[NWM_RESET]              = (ULONG) wdp_NWM_RESET;
    wdp_Methods[NWM_GETNUMPLAYERS]      = (ULONG) wdp_NWM_GETNUMPLAYERS;

    wdp_Methods[NWM_CHECKREMOTESTART]   = (ULONG) wdp_NWM_CHECKREMOTESTART;
    
    wdp_Methods[NWM_ASKLOCALMACHINE]    = (ULONG) wdp_NWM_ASKLOCALMACHINE;
    wdp_Methods[NWM_SETSESSIONIDENT]    = (ULONG) wdp_NWM_SETSESSIONIDENT;

    wdp_Methods[NWM_DIAGNOSIS]          = (ULONG) wdp_NWM_DIAGNOSIS;

    wdp_clinfo.superclassid = NETWORK_CLASSID;
    wdp_clinfo.methods      = wdp_Methods;
    wdp_clinfo.instsize     = sizeof(struct windp_data);
    wdp_clinfo.flags        = 0;

    return(&wdp_clinfo);
}

/*-----------------------------------------------------------------*/
BOOL wdp_FreeClass(void)
/*
**  CHANGED
**      07-Nov-96   floh    created
*/
{
    struct Node *nd;

    return(TRUE);
}

/*=================================================================**
**  METHODEN HANDLER                                               **
**=================================================================*/

/*-----------------------------------------------------------------*/
_dispatcher(Object *, wdp_OM_NEW, struct TagItem *attrs)
/*
**  CHANGED
**      08-Nov-96   floh    created
*/
{
    struct windp_data  *wdp;
    Object *newo;

    newo = (Object *) _supermethoda(cl,o,OM_NEW,attrs);
    if (!newo) return(NULL);
    wdp = INST_DATA(cl,newo);

    /* ----------------------------------------------------------------------
    ** Allozieren und Ausfüllen der Strukturen. Weil hier wieder WinBox-
    ** spezifisches reinspielt (zum Beispiel die größe...), machen wir es mit
    ** 2 InitRoutinen.
    ** --------------------------------------------------------------------*/
    if( wdp_InitVFMStuff( wdp ) ) {
        if( !wdp_InitWinStuff( wdp ) ) {
            _dispose( newo ); return( NULL );
            }
        }
    else {
        _dispose( newo ); return( NULL );
        }

    /*** Jetzt fragen wir gleich mal nach den Providern ***/
    _methoda( newo, NWM_ASKPROVIDERS, NULL );

    _GetConfigItems(NULL,wdp_ConfigItems,WINDP_NUM_CONFIG_ITEMS);
    wdp_SetGuaranteedMode( wdp->win_data, wdp_ConfigItems[0].data );
    wdp_SetVersionCheck( wdp->win_data,   wdp_ConfigItems[1].data );

    /*** wow, das war's schon ***/
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, wdp_OM_DISPOSE, void *nil)
/*
**  CHANGED
**      09-Nov-96   floh    created
*/
{
    struct windp_data *wdp = INST_DATA(cl,o);

    /*** Freigeben der Strukturen ***/
    wdp_FreeVFMStuff( wdp );
    wdp_FreeWinStuff( wdp );

    /*** Noch emol nach obm guckn ***/
    return((BOOL)_supermethoda(cl,o,OM_DISPOSE,nil));
}

/* ----------------------------------------------------------------
**                      Suport-Routinen
** --------------------------------------------------------------*/
BOOL wdp_InitVFMStuff( struct windp_data *wdp)
{
    if( sizeof( struct windp_vfm_data ) > 0 ) {

        /*** Es lohnt zu allozieren und der Wert hat ok zu sein ***/
        if( !(wdp->vfm_data = (struct windp_vfm_data *)
             _AllocVec( sizeof( struct windp_vfm_data ),
             MEMF_PUBLIC|MEMF_CLEAR) ) )
             return( FALSE );
        }

    /*** Ausfüllen ? ***/

    /*** Und Tschüß ***/
    return( TRUE );
}

void wdp_FreeVFMStuff( struct windp_data *wdp )
{
    /*** War was alloziert? ***/
    if( wdp->vfm_data ) {

        _FreeVec( wdp->vfm_data );
        wdp->vfm_data = NULL;
        }

    return;
}

/*-----------------------------------------------------------------*/
_dispatcher(void, wdp_OM_SET, struct TagItem *attrs)
/*
**  CHANGED
**      11-Nov-96   floh    created
*/
{
    struct windp_data *wdp = INST_DATA(cl,o);
    ULONG tag;
    struct TagItem *ti = attrs;

    while ((tag = ti->ti_Tag) != TAG_DONE) {

        ULONG data = (ULONG *) ti++->ti_Data;

        switch(tag) {
            case TAG_IGNORE:    continue;
            case TAG_MORE:      ti = (struct TagItem *) data; break;
            case TAG_SKIP:      ti += (ULONG) data; break;
            default:

                switch(tag) {

                    case NWA_GuaranteedMode:

                        /*** Daten aus WinBox holen ***/
                        wdp_SetGuaranteedMode( wdp->win_data, data );
                        break;
                    
                };
        };
    };
    _supermethoda(cl,o,OM_GET,attrs);
}

_dispatcher(void, wdp_OM_GET, struct TagItem *attrs)
/*
**  CHANGED
**      11-Nov-96   floh    created
*/
{
    struct windp_data *wdp = INST_DATA(cl,o);
    ULONG tag;
    struct TagItem *ti = attrs;

    while ((tag = ti->ti_Tag) != TAG_DONE) {

        ULONG *value = (ULONG *) ti++->ti_Data;

        switch(tag) {
            case TAG_IGNORE:    continue;
            case TAG_MORE:      ti = (struct TagItem *) value; break;
            case TAG_SKIP:      ti += (ULONG) value; break;
            default:

                switch(tag) {

                    case NWA_NumPlayers:

                        /*** Daten aus WinBox holen ***/
                        *value = (LONG) wdp_GetNumPlayers( wdp->win_data );
                        break;
                    
                };
        };
    };
    _supermethoda(cl,o,OM_GET,attrs);
}


#endif

