/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_main.c,v $
**  $Revision: 38.40 $
**  $Date: 1998/01/06 16:23:15 $
**  $Locker: floh $
**  $Author: floh $
**
**  Main-Modul der ypaworld.class
**
**  (C) Copyright 1995 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/nodes.h>
#include <exec/lists.h>
#include <utility/tagitem.h>
#include <libraries/iffparse.h>

#include <stdlib.h>
#include <string.h>

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "nucleus/nukedos.h"
#include "types.h"
#include "modules.h"
#include "engine/engine.h"

#include "visualstuff/ov_engine.h"
#include "transform/te.h"

#include "skeleton/skltclass.h"
#include "bitmap/bitmapclass.h"
#include "ypa/ypabactclass.h"
#include "ypa/ypaworldclass.h"

#include "yw_protos.h"

#ifdef __NETWORK__
#include "network/networkclass.h"
#endif

/*-------------------------------------------------------------------
**  Methoden-Prototypen
*/
_dispatcher(Object *, yw_OM_NEW, struct TagItem *attrs);
_dispatcher(BOOL, yw_OM_DISPOSE, void *ignored);
_dispatcher(void, yw_OM_SET, struct TagItem *attrs);
_dispatcher(void, yw_OM_GET, struct TagItem *attrs);

/*** yw_building.c ***/
_dispatcher(BOOL, yw_YWM_CREATEBUILDING, struct createbuilding_msg *);

/*** yw_weapon.c ***/
_dispatcher(Object *, yw_YWM_CREATEWEAPON, struct createweapon_msg *);

/*** yw_vehicle.c ***/
_dispatcher(Object *, yw_YWM_CREATEVEHICLE, struct createvehicle_msg *);

/*** yw_supp.c ***/
_dispatcher(ULONG, yw_YWM_GETSECTORINFO, struct position_msg *);
_dispatcher(void, yw_YWM_SETVIEWER, struct Bacterium *);
_dispatcher(struct Bacterium *, yw_YWM_GETVIEWER, void *ignore);
_dispatcher(void, yw_YWM_GETHEIGHT, struct getheight_msg *);
_dispatcher(Object *, yw_YWM_GETVISPROTO, ULONG *);
_dispatcher(void, yw_YWM_ADDCOMMANDER, Object *);
_dispatcher(ULONG, yw_YWM_ISVISIBLE, struct Bacterium *);
_dispatcher(void, yw_YWM_VISOR, struct visor_msg *);
_dispatcher(void, yw_YWM_NOTIFYWEAPONHIT, struct yw_notifyweaponhit_msg *);
_dispatcher(void, yw_YWM_ONLINEHELP, struct yw_onlinehelp_msg *);

/*** yw_sim.c ***/
_dispatcher(void, yw_BSM_TRIGGER, struct trigger_msg *);

/*** yw_newinit.c ***/
_dispatcher(ULONG, yw_YWM_CREATELEVEL, struct createlevel_msg *);
_dispatcher(ULONG, yw_YWM_NEWLEVEL, struct newlevel_msg *);
_dispatcher(void, yw_YWM_KILLLEVEL, void *);
_dispatcher(ULONG, yw_YWM_ADVANCEDCREATELEVEL, struct createlevel_msg *);

/*** yw_intersect.c ***/
_dispatcher(void, yw_YWM_INTERSECT, struct intersect_msg *msg);
_dispatcher(void, yw_YWM_INSPHERE, struct insphere_msg *msg);
_dispatcher(void, yw_YWM_INTERSECT2, struct intersect_msg *msg);
_dispatcher(void, yw_YWM_INBACT, struct inbact_msg *msg);

/*** yw_energy.c ***/
_dispatcher(void, yw_YWM_MODSECTORENERGY, struct energymod_msg *msg);
_dispatcher(void, yw_YWM_GETRLDRATIO, struct getrldration_msg *msg);
_dispatcher(void, yw_YWM_NOTIFYDEADROBO, struct notifydeadrobo_msg *msg);

/*** yw_gui.c ***/
_dispatcher(void, yw_YWM_ADDREQUESTER, struct YPAReq *req);
_dispatcher(void, yw_YWM_REMREQUESTER, struct YPAReq *req);

/*** yw_deathcache.c ***/
_dispatcher(Object *, yw_YWM_OBTAINVEHICLE, struct obtainvehicle_msg *msg);
_dispatcher(void, yw_YWM_RELEASEVEHICLE, Object *v_obj);

/*** yw_gameshell.c ***/
_dispatcher( BOOL, yw_YWM_INITGAMESHELL, struct GameShellReq *GSR );
_dispatcher( BOOL, yw_YWM_FREEGAMESHELL, struct GameShellReq *GSR );
_dispatcher( BOOL, yw_YWM_OPENGAMESHELL, struct GameShellReq *GSR );
_dispatcher( BOOL, yw_YWM_CLOSEGAMESHELL, struct GameShellReq *GSR );
_dispatcher( BOOL, yw_YWM_TRIGGERGAMESHELL, struct GameShellReq *GSR);
_dispatcher( BOOL, yw_YWM_REFRESHGAMESHELL, struct rsaction_msg *gsa);

/*** yw_gameloadsave.c ***/
_dispatcher( BOOL, yw_YWM_SAVEGAME, struct saveloadgame_msg *slg );
_dispatcher( BOOL, yw_YWM_LOADGAME, struct saveloadgame_msg *slg );

/*** yw_gsaction.c ***/
_dispatcher( BOOL, yw_YWM_SETGAMEINPUT, struct GameShellReq *GSR );
_dispatcher( BOOL, yw_YWM_SETGAMEVIDEO, struct GameShellReq *GSR );
_dispatcher( BOOL, yw_YWM_SETGAMELANGUAGE, struct GameShellReq *GSR );

/*** yw_gsio.c ***/
_dispatcher( BOOL, yw_YWM_LOADSETTINGS, struct saveloadsettings_msg *lsm );
_dispatcher( BOOL, yw_YWM_SAVESETTINGS, struct saveloadsettings_msg *lsm );

/*** yw_logwin.c ***/
_dispatcher(void, yw_YWM_LOGMSG, struct logmsg_msg *msg);

/*** yw_play.c ***/
_dispatcher(ULONG, yw_YWM_INITPLAYER, struct initplayer_msg *);
_dispatcher(void, yw_YWM_KILLPLAYER, void *);
_dispatcher(void, yw_YWM_PLAYERCONTROL, struct playercontrol_msg *);
_dispatcher(void, yw_YWM_TRIGGERPLAYER, struct trigger_msg *);

/*** yw_locale.c ***/
_dispatcher(ULONG, yw_YWM_SETLANGUAGE, struct setlanguage *);

/*** yw_level.c ***/
_dispatcher(ULONG, yw_YWM_BEAMNOTIFY, struct beamnotify *);

/*** yw_history.c ***/
_dispatcher(void, yw_YWM_NOTIFYHISTORYEVENT, UBYTE *);

#ifdef __WINDOWS__
/*** yw_forcefeedback.c ***/
_dispatcher(void, yw_YWM_FORCEFEEDBACK, struct yw_forcefeedback_msg *);
#endif

#ifdef __NETWORK__
/*** yw_network.c ***/
_dispatcher( BOOL, yw_YWM_NETWORKLEVEL, struct createlevel_msg *);
_dispatcher( BOOL, yw_YWM_SENDMESSAGE,  struct sendmessage_msg *);
#endif

/*-------------------------------------------------------------------
**  Globals
*/
_use_nucleus
_use_tform_engine
_use_ov_engine
_use_input_engine
_use_audio_engine

#ifdef AMIGA
__far ULONG yw_Methods[NUCLEUS_NUMMETHODS];
#else
ULONG yw_Methods[NUCLEUS_NUMMETHODS];
#endif

struct ClassInfo yw_clinfo;

/*** HACK: Pointer auf Speicherbereiche der base.class, siehe OM_NEW ***/
#ifdef AMIGA
__far struct pubstack_entry *yw_PubStack;
__far                 UBYTE *yw_ArgStack;
__far                 UBYTE *yw_EOArgStack;
#else
struct pubstack_entry *yw_PubStack;
                UBYTE *yw_ArgStack;
                UBYTE *yw_EOArgStack;
#endif


/*-------------------------------------------------------------------
**  Global Entry Table des Klassen-Moduls, plus Prototypes.
*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeYWClass(ULONG id,...);
__geta4 BOOL FreeYWClass(void);
#else
struct ClassInfo *MakeYWClass(ULONG id,...);
BOOL FreeYWClass(void);
#endif

struct GET_Class yw_GET = {
    &MakeYWClass,                 /* MakeExternClass() */
    &FreeYWClass,                 /* FreeExternClass() */
};

ULONG DeathCount;

/*===================================================================
**  *** CODESEGMENT HEADER ***
*/
#ifdef DYNAMIC_LINKING
#ifdef AMIGA
__geta4 struct GET_Class *start(void)
{
    return(&yw_GET);
}
#endif
#endif

#ifdef STATIC_LINKING
/* der Entry-Point */
struct GET_Class *yw_Entry(void)
{
    return(&yw_GET);
};

/* und die SegInfo-Struktur */
struct SegInfo yw_class_seg = {
    { NULL, NULL,                   /* ln_Succ, ln_Pred */
      0, 0,                         /* ln_Type, ln_Pri  */
      "MC2classes:ypaworld.class"   /* der Segment-Name */
    },
    yw_Entry,                       /* Entry()-Point */
};
#endif

/*-----------------------------------------------------------------*/
#ifdef DYNAMIC_LINKING
/*-------------------------------------------------------------------
**  Logischerweise kann der NUC_GET-Pointer nicht mit einem
**  _GetTagData() aus dem Nucleus-Kernel ermittelt werden,
**  weil NUC_GET noch nicht initialisiert ist! Deshalb hier eine
**  handgestrickte Routine.
*/
struct GET_Nucleus *local_GetNucleus(struct TagItem *tagList)
{
    register ULONG act_tag;

    while ((act_tag = tagList->ti_Tag) != MID_NUCLEUS) {
        switch (act_tag) {
            case TAG_DONE:  return(NULL); break;
            case TAG_MORE:  tagList = (struct TagItem *) tagList->ti_Data; break;
            case TAG_SKIP:  tagList += tagList->ti_Data; break;
        };
        tagList++;
    };
    return((struct GET_Nucleus *)tagList->ti_Data);
}
#endif /* DYNAMIC_LINKING */

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeYWClass(ULONG id,...)
#else
struct ClassInfo *MakeYWClass(ULONG id,...)
#endif
/*
**  FUNCTION
**      Meldet ypaworld.class im Nucleus-System an.
**
**  INPUTS
**      Folgende Tags werden akzeptiert:
**          MID_ENGINE_OUTPUT_VISUAL
**
**  RESULTS
**      Pointer auf ausgefnllte ClassInfo-Struktur
**
**  CHANGED
**      24-Apr-95   floh    created
**      11-Aug-95   floh    ben÷tigt jetzt auch input.engine
**      31-Dec-95   floh    - YWM_UNITMENUCONTROL
**      06-Apr-96   floh    + YWM_VISOR
**      26-Apr-96   floh    + benötigt jetzt Audio-Engine
**      29-Apr-96   floh    + AF's Gameshell-Methoden.
**      10-May-96   floh    + YWM_LOGMSG
**      08-Jul-96   floh    + YWM_CREATELEVEL
**                          + YWM_INITPLAYER
**                          + YWM_KILLPLAYER
**      24-Jul-96   floh    + YWM_SETLANGUAGE
**      25-Jul-96   floh    + YWM_REFRESHGAMESHELL
**      07-Aug-96   floh    + YWM_BEAMNOTIFY
**      23-Oct-96   floh    + AF's neue gesplittete GameShell-Methoden
**      02-Nov-96   floh    + YWM_GETRLDRATIO
**      03-Nov-96   floh    + YWM_NOTIFYDEADROBO
**      22-May-97   floh    + YWM_FORCEFEEDBACK
**      01-Aug-97   floh    + YWM_NOTIFYWEAPONHIT
**                          + YWM_ADVANCEDCREATELEVEL
**      25-Aug-97   floh    + YWM_NOTIFYHISTORYEVENT
**      05-Mar-98   floh    + YWM_ONLINEHELP
*/
{
    struct TagItem *tlist = (struct TagItem *) &id;

    #ifdef DYNAMIC_LINKING
    /* GET-Pointer aus TagArray holen */
    if (!(NUC_GET = local_GetNucleus(tlist))) return(NULL);
    #endif

    _get_tform_engine(tlist);
    _get_ov_engine(tlist);
    _get_input_engine(tlist);
    _get_audio_engine(tlist);

    /* Methoden-Array initialisieren */
    memset(yw_Methods,0,sizeof(yw_Methods));

    yw_Methods[OM_NEW]      = (ULONG) yw_OM_NEW;
    yw_Methods[OM_DISPOSE]  = (ULONG) yw_OM_DISPOSE;
    yw_Methods[OM_SET]      = (ULONG) yw_OM_SET;
    yw_Methods[OM_GET]      = (ULONG) yw_OM_GET;

    yw_Methods[BSM_TRIGGER] = (ULONG) yw_BSM_TRIGGER;

    yw_Methods[YWM_GETSECTORINFO]      = (ULONG) yw_YWM_GETSECTORINFO;
    yw_Methods[YWM_SETVIEWER]          = (ULONG) yw_YWM_SETVIEWER;
    yw_Methods[YWM_GETHEIGHT]          = (ULONG) yw_YWM_GETHEIGHT;
    yw_Methods[YWM_GETVISPROTO]        = (ULONG) yw_YWM_GETVISPROTO;
    yw_Methods[YWM_ADDCOMMANDER]       = (ULONG) yw_YWM_ADDCOMMANDER;
    yw_Methods[YWM_NEWLEVEL]           = (ULONG) yw_YWM_NEWLEVEL;
    yw_Methods[YWM_INTERSECT]          = (ULONG) yw_YWM_INTERSECT;
    yw_Methods[YWM_INSPHERE]           = (ULONG) yw_YWM_INSPHERE;
    yw_Methods[YWM_MODSECTORENERGY]    = (ULONG) yw_YWM_MODSECTORENERGY;
    yw_Methods[YWM_GETVIEWER]          = (ULONG) yw_YWM_GETVIEWER;
    yw_Methods[YWM_ADDREQUESTER]       = (ULONG) yw_YWM_ADDREQUESTER;
    yw_Methods[YWM_REMREQUESTER]       = (ULONG) yw_YWM_REMREQUESTER;
    yw_Methods[YWM_OBTAINVEHICLE]      = (ULONG) yw_YWM_OBTAINVEHICLE;
    yw_Methods[YWM_RELEASEVEHICLE]     = (ULONG) yw_YWM_RELEASEVEHICLE;
    yw_Methods[YWM_ISVISIBLE]          = (ULONG) yw_YWM_ISVISIBLE;
    yw_Methods[YWM_CREATEVEHICLE]      = (ULONG) yw_YWM_CREATEVEHICLE;
    yw_Methods[YWM_CREATEWEAPON]       = (ULONG) yw_YWM_CREATEWEAPON;
    yw_Methods[YWM_CREATEBUILDING]     = (ULONG) yw_YWM_CREATEBUILDING;
    yw_Methods[YWM_INTERSECT2]         = (ULONG) yw_YWM_INTERSECT2;
    yw_Methods[YWM_INBACT]             = (ULONG) yw_YWM_INBACT;
    yw_Methods[YWM_KILLLEVEL]          = (ULONG) yw_YWM_KILLLEVEL;
    yw_Methods[YWM_VISOR]              = (ULONG) yw_YWM_VISOR;
    yw_Methods[YWM_INITGAMESHELL]      = (ULONG) yw_YWM_INITGAMESHELL;
    yw_Methods[YWM_FREEGAMESHELL]      = (ULONG) yw_YWM_FREEGAMESHELL;
    yw_Methods[YWM_OPENGAMESHELL]      = (ULONG) yw_YWM_OPENGAMESHELL;
    yw_Methods[YWM_CLOSEGAMESHELL]     = (ULONG) yw_YWM_CLOSEGAMESHELL;
    yw_Methods[YWM_TRIGGERGAMESHELL]   = (ULONG) yw_YWM_TRIGGERGAMESHELL;
    yw_Methods[YWM_LOGMSG]             = (ULONG) yw_YWM_LOGMSG;
    yw_Methods[YWM_REALIZESHELLACTION] = (ULONG) NULL;  // OBSOLETE!
    yw_Methods[YWM_CREATELEVEL]        = (ULONG) yw_YWM_CREATELEVEL;
    yw_Methods[YWM_INITPLAYER]         = (ULONG) yw_YWM_INITPLAYER;
    yw_Methods[YWM_KILLPLAYER]         = (ULONG) yw_YWM_KILLPLAYER;
    yw_Methods[YWM_TRIGGERPLAYER]      = (ULONG) yw_YWM_TRIGGERPLAYER;
    yw_Methods[YWM_PLAYERCONTROL]      = (ULONG) yw_YWM_PLAYERCONTROL;
    yw_Methods[YWM_SETLANGUAGE]        = (ULONG) yw_YWM_SETLANGUAGE;
    yw_Methods[YWM_REFRESHGAMESHELL]   = (ULONG) yw_YWM_REFRESHGAMESHELL;
    yw_Methods[YWM_BEAMNOTIFY]         = (ULONG) yw_YWM_BEAMNOTIFY;
    yw_Methods[YWM_SAVEGAME]           = (ULONG) yw_YWM_SAVEGAME;
    yw_Methods[YWM_LOADGAME]           = (ULONG) yw_YWM_LOADGAME;
    yw_Methods[YWM_SETGAMEINPUT]       = (ULONG) yw_YWM_SETGAMEINPUT;
    yw_Methods[YWM_SETGAMEVIDEO]       = (ULONG) yw_YWM_SETGAMEVIDEO;
    yw_Methods[YWM_SETGAMELANGUAGE]    = (ULONG) yw_YWM_SETGAMELANGUAGE;
    yw_Methods[YWM_LOADSETTINGS]       = (ULONG) yw_YWM_LOADSETTINGS;
    yw_Methods[YWM_SAVESETTINGS]       = (ULONG) yw_YWM_SAVESETTINGS;
    yw_Methods[YWM_GETRLDRATIO]        = (ULONG) yw_YWM_GETRLDRATIO;
    yw_Methods[YWM_NOTIFYDEADROBO]     = (ULONG) yw_YWM_NOTIFYDEADROBO;
    #ifdef __NETWORK__
    yw_Methods[YWM_NETWORKLEVEL]       = (ULONG) yw_YWM_NETWORKLEVEL;
    yw_Methods[YWM_SENDMESSAGE]        = (ULONG) yw_YWM_SENDMESSAGE;
    #endif
    #ifdef __WINDOWS__
    yw_Methods[YWM_FORCEFEEDBACK]      = (ULONG) yw_YWM_FORCEFEEDBACK;
    #endif
    yw_Methods[YWM_NOTIFYWEAPONHIT]     = (ULONG) yw_YWM_NOTIFYWEAPONHIT;
    yw_Methods[YWM_ADVANCEDCREATELEVEL] = (ULONG) yw_YWM_ADVANCEDCREATELEVEL;
    yw_Methods[YWM_NOTIFYHISTORYEVENT]  = (ULONG) yw_YWM_NOTIFYHISTORYEVENT;
    yw_Methods[YWM_ONLINEHELP]          = (ULONG) yw_YWM_ONLINEHELP;

    /* ClassInfo-Struktur ausfüllen */
    yw_clinfo.superclassid = BASE_CLASSID;
    yw_clinfo.methods      = yw_Methods;
    yw_clinfo.instsize     = sizeof(struct ypaworld_data);
    yw_clinfo.flags        = 0;

    /* Globals initialisieren */
    DeathCount = (1<<16);

    /* fertig */
    return(&yw_clinfo);
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 BOOL FreeYWClass(void)
#else
BOOL FreeYWClass(void)
#endif
/*
**  FUNCTION
**
**  INPUTS
**      ---
**
**  RESULTS
**      TRUE -> alles OK
**      FALSE-> Klasse konnte nicht freigegeben werden (sollte eigentlich
**              nie auftreten).
**
**  CHANGED
**      24-Apr-95   floh    created
*/
{
    return(TRUE);
}

/*=================================================================**
**  METHODEN DISPATCHER                                            **
**=================================================================*/

/*-----------------------------------------------------------------*/
_dispatcher(Object *, yw_OM_NEW, struct TagItem *attrs)
/*
**  CHANGED
**      24-Apr-95   floh    created
**      01-May-95   floh    neu: SectorArray[]
**      04-May-95   floh    debugging
**      23-May-95   floh    neu: Support fnr Kollisions-Skeletons
**      28-May-95   floh    neu: yw_InitSlurp()
**      06-Jun-95   floh    vereinfachtes Sector-Visual-Type-Handling,
**                          neu: VisProtos-Array.
**      11-Jun-95   floh    Robo-List ist jetzt CmdList (fnr Commander-Objects)
**      28-Jun-95   floh    massive _nderungen wegen neuer Sektor-Organisation
**      10-Jul-95   floh    vom BeeBox-Skeleton wird jetzt das Skeleton
**                          "extrahiert"
**      11-Jul-95   floh    (1) neu: heaven.base wird geladen
**                          (2) heaven.base bekommt jetzt BSA_VisLimit
**                              und BSA_DFadeLength neu gesetzt.
**      19-Jul-95   floh    Neu: Initialisierung <EnergyMap> und
**                          Kraftwerks-Array
**      20-Jul-95   floh    benutzt jetzt yw_InitEnergyModule()
**      11-Aug-95   floh    _NewList(ReqList)
**      12-Aug-95   floh    erfragt jetzt von Gfx.Engine die X/Y-Aufl÷sung
**                          (wird ben÷tigt fnr GUI).
**      13-Aug-95   floh    debugging...
**      23-Sep-95   floh    + DeathCache Initialisierung
**      24-Sep-95   floh    + alle nicht beweglichen base.class Objekte
**                            bekommen jetzt das BSA_Static-Attribut
**                            gesetzt (schaltet Rotations-Matrix-Mul aus)
**      25-Sep-95   floh    ruft jetzt yw_InitVehicleTypes() auf
**                          (in yw_deathcache.c)
**      05-Nov-95   floh    + yw_font.c/yw_InitFontModule() Aufruf
**                          + _LogMsg()'s
**      25-Dec-95   floh    + ywd->TracyRemap und ywd->ShadeRemap wird
**                            initialisiert, erstmal nur als Hack, weil
**                            die Remap-Tabellen Set-spezifisch sind
**      29-Dec-95   floh    + yw_InitMouse()
**      24-Jan-96   floh    Massiv aufgeräumt, weil vieles jetzt in
**                          der Set-Initialisierung stattfindet.
**      24-Apr-96   floh    revised & updated
**                          - yw_InitGUIModule(), jetzt in YWM_NEWLEVEL
**                          - yw_InitEnergyModule(), jetzt in YWM_NEWLEVEL
**      03-May-96   floh    Anfangszustand der Vehicle-/Weapon-/Buildprotos 
**                          wird jetzt aus einem gemeinsamen Script
**                          namens "levels:scripts/startup.scr" geladen.
**                          (Grand Unified Script Handling).
**      24-May-96   floh    + Backpointer auf Object itself
**      22-Jun-96   floh    + Initialisierung sqrt-Tabelle für Kraftwerks-
**                            Füller.
**      07-Jul-96   floh    + yw_InitSceneRecorder()
**      24-Jul-96   floh    + Initialisierung des Locale-Systems
**      07-Aug-96   floh    + yw_InitLevelInfo()
**      29-Aug-96   floh    + yw_InitLevelInfo() -> yw_InitLevelNet()
**      17-Sep-96   floh    + yw_InitTooltips()
**      21-Sep-96   floh    + explizite Designer-Init
**      27-Sep-96   floh    + weil ich mal wieder an 64kByte-Grenze
**                            für die Instance Data gekommen bin,
**                            werden die 4 größten internen Arrays
**                            jetzt dynamisch allokiert!
**      19-Jun-97   floh    + YPA_PREFS_SOUNDENABLE wird defaultmäßig
**                            angeschaltet
**      23-Mar-98   floh    + temporäre Assign-Initialisierung
**      04-Apr-98   floh    + Display-Resolution-Zeug wird initialisiert
**      24-May-98   floh    + initialisiert Default-Sprache "language"
**                          + macht CD Check fuer Typical und Minimum
**                            Installation
*/
{
    Object *newo;
    struct ypaworld_data *ywd;
    ULONG xres, yres;
    ULONG i,j;
    struct setlanguage_msg slm;

    /* Methode an Superclass */
    newo = (Object *) _supermethoda(cl,o,OM_NEW,(Msg *)attrs);
    if (!newo) {
        _LogMsg("yw_main.c/OM_NEW: _supermethoda() failed!\n");
        return(NULL);
    };

    /* LID-Pointer */
    ywd = INST_DATA(cl,newo);

    /*** Backpointer ***/
    ywd->world = newo;
    
    /*** Assigns initialisieren ***/
    _SetSysPath(SYSPATH_RESOURCES,"rsrc:");
    _SetAssign("rsrc","mc2res");
    _SetAssign("data","Data");
    _SetAssign("save","Save");
    _SetAssign("help","Help");
    _SetAssign("mov","Data:mov");
    _SetAssign("levels","Levels");
    _SetAssign("mbpix","levels:mbpix");
    if (!yw_ParseAssignScript("env/assign.txt")) {
        _LogMsg("Warning, no env/assign.txt script.\n");
    };
    yw_ParseAssignRegistryKeys();
    
    /*** Locale-System initialisieren ***/
    if (!yw_InitLocale(ywd)) {
        _LogMsg("yw_main.c/OM_NEW: yw_InitLocale() failed!\n");
        _methoda(newo, OM_DISPOSE, NULL);
        return(NULL);
    };
    slm.lang = "language";
    _methoda(o,YWM_SETLANGUAGE,&slm);    
    
    /*** CD-Check ***/
    if (!yw_CheckCD(TRUE,TRUE,
            ypa_GetStr(ywd->LocHandle,STR_APPNAME,"YOUR PERSONAL AMOK"),
            ypa_GetStr(ywd->LocHandle,STR_CDREQUEST_BODYTEXT,"THE YPA CD IS REQUIRED FOR COMPACT AND TYPICAL INSTALL.")))
    {
        _methoda(newo, OM_DISPOSE, NULL);
        return(NULL);
    };

    /*** alloziere Arrays ***/
    ywd->VisProtos = (struct VisProto *)
        _AllocVec(MAXNUM_VISPROTOS*sizeof(struct VisProto), MEMF_PUBLIC|MEMF_CLEAR);
    ywd->Legos     = (struct Lego *)
        _AllocVec(MAXNUM_LEGOS*sizeof(struct Lego), MEMF_PUBLIC|MEMF_CLEAR);
    ywd->SubSects  = (struct SubSectorDesc *)
        _AllocVec(MAXNUM_SUBSECTS*sizeof(struct SubSectorDesc), MEMF_PUBLIC|MEMF_CLEAR);
    ywd->Sectors   = (struct SectorDesc *)
        _AllocVec(MAXNUM_SECTORS*sizeof(struct SectorDesc), MEMF_PUBLIC|MEMF_CLEAR);

    if (!(ywd->VisProtos && ywd->Legos && ywd->SubSects && ywd->Sectors)) {
        _methoda(newo, OM_DISPOSE, NULL);
        return(NULL);
    };

    /*** Anfangs-Zustand für die Prototypes-Arrays herstellen.    ***/
    if (!yw_InitPrototypeArrays(ywd)) {
        _LogMsg("ERROR: couldn't initialize prototypes.\n");
        _methoda(newo, OM_DISPOSE, NULL);
        return(NULL);
    };

    /*** Pointer auf Pub/Arg-Stack-Bereiche der base.class besorgen. ***/
    /*** Das ist mehr ein Hack, um Speicher fnr die doch ziemlich    ***/
    /*** gro_en Pub- und ArgStacks zu sparen. Siehe yw_render.c fnr  ***/
    /*** Verwendung dieser Pointer!                                  ***/

    _method(newo, OM_GET, BSA_PubStack,   &yw_PubStack,
                          BSA_ArgStack,   &yw_ArgStack,
                          BSA_EOArgStack, &yw_EOArgStack,
                          TAG_DONE);

    /*=====================**
    **  LID initialisieren **
    **=====================*/

    /*** X/Y-Display-Aufl÷sung ermitteln ***/
    _OVE_GetAttrs(OVET_XRes, &xres,
                  OVET_YRes, &yres,
                  TAG_DONE);
    ywd->DspXRes = xres;
    ywd->DspYRes = yres;

    /*** Listen initialisieren ***/
    _NewList((struct List *) &(ywd->CmdList));
    _NewList((struct List *) &(ywd->ReqList));
    _NewList((struct List *) &(ywd->DeathCache));

    /*** (I)-Attributen erledigt ***/
    if (!yw_initAttrs(newo,ywd,attrs)) {
        _LogMsg("yw_main.c/OM_NEW: yw_initAttrs() failed!\n");
        _methoda(newo, OM_DISPOSE, NULL);
        return(NULL);
    };

    /*** Energy2Lego-Transformations-Tabelle bauen ***/
    i=0;
    ywd->Energy2Lego[i++] = DLINDEX_EMPTY;
    for (i; i<100; i++) ywd->Energy2Lego[i] = DLINDEX_DESTROYED;
    for (i; i<200; i++) ywd->Energy2Lego[i] = DLINDEX_DAMAGED;
    for (i; i<256; i++) ywd->Energy2Lego[i] = DLINDEX_FULL;

    /*** Distanz-Tabelle für Kraftwerks-Füller aufbauen ***/
    for (j=0; j<64; j++) {
        for (i=0; i<64; i++) {
            FLOAT d = nc_sqrt((FLOAT)(i*i+j*j));
            BYTE v = (BYTE) d;
            ywd->DistTable[j*64+i] = v;
        };
    };

    /*** CellArea allokieren ***/
    ywd->CellArea = (struct Cell *) 
        _AllocVec(ywd->MapMaxX * ywd->MapMaxY * sizeof(struct Cell),
        MEMF_PUBLIC|MEMF_CLEAR);

    if (ywd->CellArea == NULL) {
        _LogMsg("yw_main.c/OM_NEW: alloc of cell area failed!\n");
        _methoda(newo, OM_DISPOSE, NULL);
        return(NULL);
    };

    /*** Scene-Recorder initialisieren ***/
    if (!yw_InitSceneRecorder(ywd)) {
        _LogMsg("yw_main.c/OM_NEW: init scene recorder failed!\n");
        _methoda(newo, OM_DISPOSE, NULL);
        return(NULL);
    };

    /*** Tooltip-System initialisieren ***/
    if (!yw_InitTooltips(ywd)) {
        _LogMsg("yw_main.c/OM_NEW: yw_InitTooltips() failed!\n");
        _methoda(newo, OM_DISPOSE, NULL);
        return(NULL);
    };

    #ifdef YPA_DESIGNMODE
    /*** Designer-Init ***/
    if (!yw_InitDesigner(ywd)) {
        _LogMsg("yw_main.c/OM_NEW: yw_InitDesigner() failed!\n");
        _methoda(newo, OM_DISPOSE, NULL);
        return( NULL );
    };
    #endif

    /*** Pre-world.ini-Parse-Initialisierung ***/
    ywd->OneDisplayRes   = TRUE;
    ywd->ShellRes        = GFX_SHELL_RESOLUTION;
    ywd->GameRes         = GFX_GAME_DEFAULT_RES;

    /*** Level-Netzwerk-Zeugs initialisieren ***/
    if (!yw_InitLevelNet(ywd)) {
        _LogMsg("yw_main.c/OM_NEW: yw_InitLevelNet() failed!\n");
        _methoda(newo, OM_DISPOSE, NULL);
        return( NULL );
    };

    /*** Prefs definieren ***/
    ywd->Prefs.Flags |= YPA_PREFS_SOUNDENABLE;

    #ifdef __NETWORK__
    if (!yw_InitNetwork(ywd)) {
        _LogMsg("yw_main.c/OM_NEW: yw_InitNetwork() failed!\n");
        _methoda(newo, OM_DISPOSE, NULL);
        return( NULL );
    };
    #endif

    /*** Misc Stuff ***/
    ywd->DontRender         = FALSE;
    ywd->UseSystemTextInput = FALSE;

    /*** Ende ***/
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, yw_OM_DISPOSE, void *ignored)
/*
**  CHANGED
**      24-Apr-95   floh    created
**      01-May-95   floh    neu: SectorArray
**      05-May-95   floh    Bugfix: beim Freigeben der base.class Objects
**                          fnr Sektor-Typen-Aussehen.
**      23-May-95   floh    neu: Support fnr Kollisions-Skeletons
**      28-May-95   floh    neu: yw_KillSlurp()
**      06-Jun-95   floh    diverse Umstrukturierungen und Vereinfachungen
**                          neu: VisProtos-Array
**      11-Jun-95   floh    alle Objects in der Command-List werden
**                          jetzt freigegeben.
**      28-Jun-95   floh    massive _nderungen wegen neuer Sektor-Organisation
**      11-Jul-95   floh    neu: Heaven-Object wird disposed
**      19-Jul-95   floh    EnergyMap und Kraftwerks-Array wird freigegeben
**      20-Jul-95   floh    benutzt jetzt yw_KillEnergyModule
**      13-Aug-95   floh    yw_KillGUIModule()
**      23-Sep-95   floh    neu: Objekte im DeathCache werden disposed.
**      05-Nov-95   floh    + yw_font.c/yw_KillFontModule()
**      25-Dec-95   floh    + _dispose(ywd->TracyRemap)
**                          + _dispose(ywd->ShadeRemap)
**                          (Achtung Hack, ist normalerweise Set-
**                          spezifisch)
**      29-Dec-95   floh    + yw_KillMouse()
**      24-Jan-96   floh    massiv aufgeräumt, siehe OM_NEW
**      27-Jan-96   floh    YWM_KILLLEVEL wird aufgerufen, für den
**                          fall, daß noch einer geladen war...
**      24-Apr-96   floh    - yw_KillGUIModule(), wird innerhalb
**                            YWM_KILLLEVEL aufgerufen
**                          - YWM_KILLLEVEL aus OM_DISPOSE entfernt
**      07-Jul-96   floh    + yw_KillSceneRecorder()
**      24-Jul-96   floh    + yw_KillLocale()
**      07-Aug-96   floh    + yw_KillLevelInfo()
**      29-Aug-96   floh    + yw_InitLevelInfo() -> yw_InitLevelNet()
**      17-Sep-96   floh    + yw_KillTooltips()
**      21-Sep-96   floh    + yw_KillDesigner()
**      27-Sep-96   floh    + die (neu) dynamisch allokierten Arrays
**                            werden freigegeben
**      12-Apr-97   floh    + yw_KillPrototypeArrays()
**      25-Aug-97   floh    + yw_KillHistory()
**      01-Sep-97   floh    + TypeMapBU und OwnerMapBU werden gekillt,
**                            falls vorhanden (das sind die Map-Backups
**                            fürs Debriefing)
**      23-Sep-97   floh    + falls existent, werden die Maps gekillt
**                            (es kann passieren, daß diese vom Debriefing,
**                            oder abgebrochenen Briefings übrigbleiben)
*/
{
    struct ypaworld_data *ywd = INST_DATA(cl,o);

    #ifdef __NETWORK__
    yw_KillNetwork( ywd );
    #endif

    #ifdef YPA_DESIGNMODE
    /*** Designer killen ***/
    yw_KillDesigner(ywd);
    #endif

    /*** Safety Cleanup: Debriefing History ***/
    yw_KillHistory(ywd);

    /*** Level-Netzwerk-Zeugs freigeben ***/
    yw_KillLevelNet(ywd);

    /*** Tooltip-System killen ***/
    yw_KillTooltips(ywd);

    /*** Locale-System killen ***/
    yw_KillLocale(ywd);

    /*** Scene-Recorder killen ***/
    yw_KillSceneRecorder(ywd);

    /*** Cell-Array freigeben ***/
    if (ywd->CellArea) _FreeVec(ywd->CellArea);

    /*** Arrays freigeben ***/
    yw_KillPrototypeArrays(ywd);
    if (ywd->Sectors) {
        _FreeVec(ywd->Sectors);  ywd->Sectors = NULL;
    };
    if (ywd->SubSects) {
        _FreeVec(ywd->SubSects);  ywd->SubSects = NULL;
    };
    if (ywd->Legos) {
        _FreeVec(ywd->Legos);  ywd->Legos = NULL;
    };
    if (ywd->VisProtos) {
        _FreeVec(ywd->VisProtos);  ywd->VisProtos = NULL;
    };
    if (ywd->TypeMapBU) {
        _dispose(ywd->TypeMapBU); ywd->TypeMapBU = NULL;
    };
    if (ywd->OwnerMapBU) {
        _dispose(ywd->OwnerMapBU); ywd->OwnerMapBU = NULL;
    };
    if (ywd->TypeMap) {
        _dispose(ywd->TypeMap); ywd->TypeMap = NULL;
    };
    if (ywd->OwnerMap) {
        _dispose(ywd->OwnerMap); ywd->OwnerMap = NULL;
    };
    if (ywd->BuildMap) {
        _dispose(ywd->BuildMap); ywd->BuildMap = NULL;
    };
    if (ywd->HeightMap) {
        _dispose(ywd->HeightMap); ywd->HeightMap = NULL;
    };

    /*** OM_DISPOSE hochreichen ***/
    return((BOOL)_supermethoda(cl,o,OM_DISPOSE,(Msg *)ignored));
}

/*-----------------------------------------------------------------*/
_dispatcher(void, yw_OM_SET, struct TagItem *attrs)
/*
**  FUNCTION
**
**  INPUTS
**
**  RESULTS
**      ---
**
**  CHANGED
**      24-Apr-95   floh    created
*/
{
    /* Attribute setzen */
    yw_setAttrs(o,INST_DATA(cl,o),attrs);

    /* und die Superclass auch... */
    _supermethoda(cl,o,OM_SET,(Msg *)attrs);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, yw_OM_GET, struct TagItem *attrs)
/*
**  FUNCTION
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      24-Apr-95   floh    created
*/
{
    /* Attribute getten */
    yw_getAttrs(o,INST_DATA(cl,o),attrs);

    /* Methode an Superclass */
    _supermethoda(cl,o,OM_GET,(Msg *)attrs);
}


