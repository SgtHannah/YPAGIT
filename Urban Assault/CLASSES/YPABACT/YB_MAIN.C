/*
**  $Source: PRG:MovingCubesII/Classes/_YPABactClass/yb_main.c,v $
**  $Revision: 38.1 $
**  $Date: 1995/06/08 23:12:41 $
**  $Locker: floh $
**  $Author: floh $
**
**  Main-Modul der ypabact.class.
**  Die ypabact.class ist Superklasse aller Vehikel im Spiel.
**
**  (C) Copyright 1995 by  Andreas Flemming
*/
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/nodes.h>
#include <exec/lists.h>
#include <utility/tagitem.h>

#include <stdlib.h>
#include <string.h>

#include "nucleus/nucleus2.h"
#include "modules.h"
#include "engine/engine.h"
#include "types.h"
#include "modules.h"

#include "transform/te.h"

#include "ypa/ypaworldclass.h"
#include "ypa/ypabactclass.h"

/*-------------------------------------------------------------------
**  Methoden-Prototypen
*/

/*** yb_main.c ***/
_dispatcher(Object *, yb_OM_NEW, struct TagItem *attrs);
_dispatcher(BOOL, yb_OM_DISPOSE, void *ignored);
_dispatcher(void, yb_OM_SET, struct TagItem *attrs);
_dispatcher(void, yb_OM_GET, struct TagItem *attrs);

/*** yb_trigger.c ***/
_dispatcher(void, yb_YBM_TR_LOGIC, struct trigger_logic_msg *msg);
_dispatcher(void, yb_YBM_TR_VISUAL, struct basepublish_msg *msg);
_dispatcher(void, yb_YBM_SETFOOTPRINT, void *nix);

/*** yb_user.c ***/
_dispatcher(void, yb_YBM_HANDLEINPUT, struct trigger_logic_msg *msg);
_dispatcher(BOOL, yb_YBM_VISIER, struct visier_msg *msg );

/*** yb_intelligence_ai12.c ***/
_dispatcher(void, yb_YBM_AI_LEVEL1, struct trigger_logic_msg *msg);
_dispatcher(void, yb_YBM_AI_LEVEL2, struct trigger_logic_msg *msg);
_dispatcher(void, yb_YBM_GETSECTARGET, struct getsectar_msg *msg);
_dispatcher(void, yb_YBM_GETFORMATIONPOS, struct getformationpos_msg *gfp);

/*** yb_intelligence_ai3.c ***/
_dispatcher(void, yb_YBM_AI_LEVEL3, struct trigger_logic_msg *msg);
_dispatcher(void, yb_YBM_ASSEMBLESLAVES, void *nix );
_dispatcher(void, yb_YBM_DOWHILECREATE, struct trigger_logic_msg *msg );
_dispatcher(void, yb_YBM_DOWHILEBEAM, struct trigger_logic_msg *msg );
_dispatcher(void, yb_YBM_DOWHILEDEATH, struct trigger_logic_msg *msg );

/*** yb_move.c ***/
_dispatcher(void, yb_YBM_MOVE, struct move_msg *move);
_dispatcher(void, yb_YBM_IMPULSE, struct impulse_msg *imp);
_dispatcher(BOOL, yb_YBM_CRASHBACTERIUM, struct crash_msg *msg);
_dispatcher(void, yb_YBM_RECOIL, struct recoil_msg *rec);
_dispatcher(BOOL, yb_YBM_CHECKLANDPOS, void *nix);
_dispatcher(void, yb_YBM_STOPMACHINE, struct trigger_logic_msg *msg );

/*** yb_die.c ***/
_dispatcher(void, yb_YBM_DIE, void *nix);
_dispatcher(void, yb_YBM_REINCARNATE, void *nix );
_dispatcher(void, yb_YBM_RELEASEVEHICLE, Object *dv );

/*** yb_masterslave.c ***/
_dispatcher(void, yb_YBM_ADDSLAVE, Object *slave);
_dispatcher(void, yb_YBM_NEWMASTER, struct newmaster_msg *msg);

/*** yb_target.c ***/
_dispatcher(void,  yb_YBM_SETTARGET, struct settarget_msg *msg);
_dispatcher(ULONG, yb_YBM_ASSESSTARGET, struct assesstarget_msg *msg);
_dispatcher(BOOL,  yb_YBM_TESTSECTARGET, struct Bacterium *enemy);

/*** yb_fight.c ***/
_dispatcher(void, yb_YBM_FIGHTBACT, struct fight_msg *fight);
_dispatcher(void, yb_YBM_FIGHTSECTOR, struct fight_msg *fight);
_dispatcher(BOOL, yb_YBM_FIREMISSILE, struct firemissile_msg *fire);
_dispatcher(void, yb_YBM_FIREYOURGUNS, struct fireyourguns_msg *fyg);
_dispatcher(void, yb_YBM_GETBESTSUB, struct flt_triple *pos);
_dispatcher(BOOL, yb_YBM_CHECKAUTOFIREPOS, struct checkautofirepos_msg *msg);

/*** yb_position.c ***/
_dispatcher(BOOL, yb_YBM_SETPOSITION, struct setposition_msg *msg);
_dispatcher(void, yb_YBM_CHECKPOSITION, void *nix );
_dispatcher(void, yb_YBM_CORRECTPOSITION, void *nix );

/*** yb_condition.c ***/
_dispatcher(void, yb_YBM_SUMPARAMETER, struct sumparamater_msg *sum);
_dispatcher(void, yb_YBM_SETSTATE, struct setstate_msg *msg);
_dispatcher(void, yb_YBM_SETSTATE_I, struct setstate_msg *msg);
_dispatcher(BOOL, yb_YBM_TESTDESTROY, void *nix );
_dispatcher(BOOL, yb_YBM_HOWDOYOUDO, void *nix );
_dispatcher(void, yb_YBM_BREAKFREE, struct trigger_logic_msg *msg);
_dispatcher(void, yb_YBM_RUNEFFECTS, void *nix);

/*** yb_energy.c ***/
_dispatcher(void, yb_YBM_GENERALENERGY, struct trigger_logic_msg *msg);
_dispatcher(void, yb_YBM_MODVEHICLEENERGY, struct modvehicleenergy_msg *msg);
_dispatcher(void, yb_YBM_MODSECTORENERGY, struct energymod_msg *msg);

/*** support.c ***/
_dispatcher(BOOL, yb_YBM_CRASHTEST, struct flt_triple *vec);
_dispatcher(void, yb_YBM_CHECKFORCERELATION, struct checkforcerelation_msg *cfr);
_dispatcher(void, yb_YBM_GIVEENEMY, struct giveenemy_msg *ge);
_dispatcher(void, yb_YBM_MERGE, Object *sl);
_dispatcher(void, yb_YBM_HANDLEVISCHILDREN, struct handlevischildren_msg *hvc );
_dispatcher(void, yb_YBM_ORGANIZE, struct organize_msg *org );

/*** bactcollision.c ***/
_dispatcher(BOOL, yb_YBM_BACTCOLLISION, struct bactcollision_msg *msg);

/*** yb_think.c ***/
_dispatcher(BOOL, yb_YBM_CANYOUDOIT, struct canyoudoit_msg *cydi);

/*** yt_findpath.c ***/
_dispatcher( BOOL, yb_YBM_FINDPATH, struct findpath_msg *fp );
_dispatcher( BOOL, yb_YBM_SETWAY, struct findpath_msg *fp );

#ifdef __NETWORK__
/*** yb_network.c ***/
_dispatcher(void, yb_YBM_TR_NETLOGIC, struct trigger_logic_msg *msg);
_dispatcher(void, yb_YBM_SHADOWTRIGGER, struct trigger_logic_msg *msg);
_dispatcher(void, yb_YBM_SHADOWTRIGGER_I, struct trigger_logic_msg *msg);
_dispatcher(void, yb_YBM_SHADOWTRIGGER_E, struct trigger_logic_msg *msg);
#endif

/*-------------------------------------------------------------------
**  externe Prototypes
*/
BOOL yb_initAttrs(Object *, struct ypabact_data *, struct TagItem *);
void yb_setAttrs(Object *, struct ypabact_data *, struct TagItem *);
void yb_getAttrs(Object *, struct ypabact_data *, struct TagItem *);

/*-------------------------------------------------------------------
**  Globals
*/
_use_nucleus
_use_tform_engine
_use_audio_engine
UBYTE **BactLocaleHandle;


#ifdef AMIGA
__far ULONG yb_Methods[NUCLEUS_NUMMETHODS];
#else
ULONG yb_Methods[NUCLEUS_NUMMETHODS];
#endif

struct ClassInfo yb_clinfo;
ULONG global_count;

/*-------------------------------------------------------------------
**  Global Entry Table des Klassen-Moduls, plus Prototypes.
*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeYBClass(ULONG id,...);
__geta4 BOOL FreeYBClass(void);
#else
struct ClassInfo *MakeYBClass(ULONG id,...);
BOOL FreeYBClass(void);
#endif

struct GET_Class yb_GET = {
    &MakeYBClass,                 /* MakeExternClass() */
    &FreeYBClass,                 /* FreeExternClass() */
};

/*===================================================================
**  *** CODESEGMENT HEADER ***
*/
#ifdef DYNAMIC_LINKING
#ifdef AMIGA
__geta4 struct GET_Class *start(void)
{
    return(&yb_GET);
}
#endif
#endif

#ifdef STATIC_LINKING
/* der Entry-Point */
struct GET_Class *yb_Entry(void)
{
    return(&yb_GET);
};

/* und die SegInfo-Struktur */
struct SegInfo yb_class_seg = {
    { NULL, NULL,                   /* ln_Succ, ln_Pred */
      0, 0,                         /* ln_Type, ln_Pri  */
      "MC2classes:ypabact.class"    /* der Segment-Name */
    },
    yb_Entry,                       /* Entry()-Point */
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
__geta4 struct ClassInfo *MakeYBClass(ULONG id,...)
#else
struct ClassInfo *MakeYBClass(ULONG id,...)
#endif
/*
**  FUNCTION
**      Meldet ypaworld.class im Nucleus-System an.
**
**  INPUTS
**
**  RESULTS
**      Pointer auf ausgefüllte ClassInfo-Struktur
**
**  CHANGED
**      08-Jun-95   floh    created
*/
{
    struct TagItem *tlist = (struct TagItem *) &id;

    #ifdef DYNAMIC_LINKING
    /* GET-Pointer aus TagArray holen */
    if (!(NUC_GET = local_GetNucleus(tlist))) return(NULL);
    #endif

    _get_tform_engine(tlist);

    /* Methoden-Array initialisieren */
    memset(yb_Methods,0,sizeof(yb_Methods));

    yb_Methods[OM_NEW]                  = (ULONG) yb_OM_NEW;
    yb_Methods[OM_DISPOSE]              = (ULONG) yb_OM_DISPOSE;
    yb_Methods[OM_SET]                  = (ULONG) yb_OM_SET;
    yb_Methods[OM_GET]                  = (ULONG) yb_OM_GET;

    yb_Methods[YBM_TR_LOGIC]            = (ULONG) yb_YBM_TR_LOGIC;
    yb_Methods[YBM_TR_VISUAL]           = (ULONG) yb_YBM_TR_VISUAL;
    yb_Methods[YBM_HANDLEINPUT]         = (ULONG) yb_YBM_HANDLEINPUT;
    yb_Methods[YBM_ADDSLAVE]            = (ULONG) yb_YBM_ADDSLAVE;
    yb_Methods[YBM_NEWMASTER]           = (ULONG) yb_YBM_NEWMASTER;
    yb_Methods[YBM_AI_LEVEL1]           = (ULONG) yb_YBM_AI_LEVEL1;
    yb_Methods[YBM_AI_LEVEL2]           = (ULONG) yb_YBM_AI_LEVEL2;
    yb_Methods[YBM_AI_LEVEL3]           = (ULONG) yb_YBM_AI_LEVEL3;
    yb_Methods[YBM_MOVE]                = (ULONG) yb_YBM_MOVE;
    yb_Methods[YBM_SETTARGET]           = (ULONG) yb_YBM_SETTARGET;
    yb_Methods[YBM_FIGHTBACT]           = (ULONG) yb_YBM_FIGHTBACT;
    yb_Methods[YBM_FIGHTSECTOR]         = (ULONG) yb_YBM_FIGHTSECTOR;
    yb_Methods[YBM_DIE]                 = (ULONG) yb_YBM_DIE;
    yb_Methods[YBM_SETSTATE]            = (ULONG) yb_YBM_SETSTATE;
    yb_Methods[YBM_FIREMISSILE]         = (ULONG) yb_YBM_FIREMISSILE;
    yb_Methods[YBM_SETPOSITION]         = (ULONG) yb_YBM_SETPOSITION;
    yb_Methods[YBM_SUMPARAMETER]        = (ULONG) yb_YBM_SUMPARAMETER;
    yb_Methods[YBM_GENERALENERGY]       = (ULONG) yb_YBM_GENERALENERGY;
    yb_Methods[YBM_IMPULSE]             = (ULONG) yb_YBM_IMPULSE;
    yb_Methods[YBM_MODVEHICLEENERGY]    = (ULONG) yb_YBM_MODVEHICLEENERGY;
    yb_Methods[YBM_CRASHTEST]           = (ULONG) yb_YBM_CRASHTEST;
    yb_Methods[YBM_CRASHBACTERIUM]      = (ULONG) yb_YBM_CRASHBACTERIUM;
    yb_Methods[YBM_BACTCOLLISION]       = (ULONG) yb_YBM_BACTCOLLISION;
    yb_Methods[YBM_RECOIL]              = (ULONG) yb_YBM_RECOIL;
    yb_Methods[YBM_CHECKLANDPOS]        = (ULONG) yb_YBM_CHECKLANDPOS;
    yb_Methods[YBM_GETSECTARGET]        = (ULONG) yb_YBM_GETSECTARGET;
    yb_Methods[YBM_GETBESTSUB]          = (ULONG) yb_YBM_GETBESTSUB;
    yb_Methods[YBM_CHECKFORCERELATION]  = (ULONG) yb_YBM_CHECKFORCERELATION;
    yb_Methods[YBM_GIVEENEMY]           = (ULONG) yb_YBM_GIVEENEMY;
    yb_Methods[YBM_GETFORMATIONPOS]     = (ULONG) yb_YBM_GETFORMATIONPOS;
    yb_Methods[YBM_CANYOUDOIT]          = (ULONG) yb_YBM_CANYOUDOIT;
    yb_Methods[YBM_REINCARNATE]         = (ULONG) yb_YBM_REINCARNATE;
    yb_Methods[YBM_STOPMACHINE]         = (ULONG) yb_YBM_STOPMACHINE;
    yb_Methods[YBM_ASSEMBLESLAVES]      = (ULONG) yb_YBM_ASSEMBLESLAVES;
    yb_Methods[YBM_DOWHILECREATE]       = (ULONG) yb_YBM_DOWHILECREATE;
    yb_Methods[YBM_TESTDESTROY]         = (ULONG) yb_YBM_TESTDESTROY;
    yb_Methods[YBM_CHECKAUTOFIREPOS]    = (ULONG) yb_YBM_CHECKAUTOFIREPOS;
    yb_Methods[YBM_SETFOOTPRINT]        = (ULONG) yb_YBM_SETFOOTPRINT;
    yb_Methods[YBM_MERGE]               = (ULONG) yb_YBM_MERGE;
    yb_Methods[YBM_BREAKFREE]           = (ULONG) yb_YBM_BREAKFREE;
    yb_Methods[YBM_FIREYOURGUNS]        = (ULONG) yb_YBM_FIREYOURGUNS;
    yb_Methods[YBM_VISIER]              = (ULONG) yb_YBM_VISIER;
    yb_Methods[YBM_HANDLEVISCHILDREN]   = (ULONG) yb_YBM_HANDLEVISCHILDREN;
    yb_Methods[YBM_HOWDOYOUDO]          = (ULONG) yb_YBM_HOWDOYOUDO;
    yb_Methods[YBM_ORGANIZE]            = (ULONG) yb_YBM_ORGANIZE;
    yb_Methods[YBM_ASSESSTARGET]        = (ULONG) yb_YBM_ASSESSTARGET;
    yb_Methods[YBM_TESTSECTARGET]       = (ULONG) yb_YBM_TESTSECTARGET;
    yb_Methods[YBM_DOWHILEBEAM]         = (ULONG) yb_YBM_DOWHILEBEAM;
    yb_Methods[YBM_RUNEFFECTS]          = (ULONG) yb_YBM_RUNEFFECTS;
    yb_Methods[YBM_CHECKPOSITION]       = (ULONG) yb_YBM_CHECKPOSITION;
    yb_Methods[YBM_CORRECTPOSITION]     = (ULONG) yb_YBM_CORRECTPOSITION;
    yb_Methods[YBM_RELEASEVEHICLE]      = (ULONG) yb_YBM_RELEASEVEHICLE;
    yb_Methods[YBM_SETSTATE_I]          = (ULONG) yb_YBM_SETSTATE_I;
    yb_Methods[YBM_MODSECTORENERGY]     = (ULONG) yb_YBM_MODSECTORENERGY;
    yb_Methods[YBM_DOWHILEDEATH]        = (ULONG) yb_YBM_DOWHILEDEATH;
    yb_Methods[YBM_FINDPATH]            = (ULONG) yb_YBM_FINDPATH;
    yb_Methods[YBM_SETWAY]              = (ULONG) yb_YBM_SETWAY;

    #ifdef __NETWORK__
    yb_Methods[YBM_TR_NETLOGIC]         = (ULONG) yb_YBM_TR_NETLOGIC;
    yb_Methods[YBM_SHADOWTRIGGER]       = (ULONG) yb_YBM_SHADOWTRIGGER;
    yb_Methods[YBM_SHADOWTRIGGER_E]     = (ULONG) yb_YBM_SHADOWTRIGGER_E;
    yb_Methods[YBM_SHADOWTRIGGER_I]     = (ULONG) yb_YBM_SHADOWTRIGGER_I;
    #endif


    /* ClassInfo-Struktur ausfüllen */
    yb_clinfo.superclassid = NUCLEUSCLASSID;
    yb_clinfo.methods      = yb_Methods;
    yb_clinfo.instsize     = sizeof(struct ypabact_data);
    yb_clinfo.flags        = 0;

    /* Zähler */
    global_count = 1;

    /* fertig */
    return(&yb_clinfo);
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 BOOL FreeYBClass(void)
#else
BOOL FreeYBClass(void)
#endif
/*
**  FUNCTION
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      08-Jun-95   floh    created
*/
{
    return(TRUE);
}

/*=================================================================**
**  METHODEN DISPATCHER                                            **
**=================================================================*/

/*-----------------------------------------------------------------*/
_dispatcher(Object *, yb_OM_NEW, struct TagItem *attrs)
/*
**  FUNCTION
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      08-Jun-95   floh    created
**      11-Jun-95   floh    Anmeldung ans Welt-Object vereinfacht.
**      25-Sep-95   floh    Bacterium-BactClassID wird ausgefüllt
*/
{
    Object *newo;
    struct ypabact_data *ybd;
    struct getsectorinfo_msg gsi_msg;
    LONG   MX, MY;

    /*** Methode an Superclass ***/
    newo = (Object *) _supermethoda(cl,o,OM_NEW,attrs);
    if (!newo) return(NULL);

    /*** LID-Pointer ***/
    ybd = INST_DATA(cl,newo);

    /*** Listen initialisieren ***/
    _NewList((struct List *) &(ybd->attck_list));
    _NewList((struct List *) &(ybd->bact.slave_list));
    _NewList((struct List *) &(ybd->bact.auto_list));

    /*** andere Initialisierungen ***/
    ybd->bact.BactClassID = BCLID_YPABACT;

    ybd->bact.ident  = global_count;
    global_count++;

    ybd->pt_attck_node.o      = newo;
    ybd->st_attck_node.o      = newo;
    ybd->bact.slave_node.o    = newo;
    ybd->pt_attck_node.bact   = &(ybd->bact);
    ybd->st_attck_node.bact   = &(ybd->bact);
    ybd->bact.slave_node.bact = &(ybd->bact);
    ybd->bact.BactObject      = newo;
    ybd->bact.ManValue        = 0.0;
    ybd->bact.robo            = NULL;

    ybd->bact.dir.m11 = ybd->bact.Viewer.dir.m11 = 1.0;
    ybd->bact.dir.m12 = ybd->bact.Viewer.dir.m12 = 0.0;
    ybd->bact.dir.m13 = ybd->bact.Viewer.dir.m13 = 0.0;

    ybd->bact.dir.m21 = ybd->bact.Viewer.dir.m21 = 0.0;
    ybd->bact.dir.m22 = ybd->bact.Viewer.dir.m22 = 1.0;
    ybd->bact.dir.m23 = ybd->bact.Viewer.dir.m23 = 0.0;

    ybd->bact.dir.m31 = ybd->bact.Viewer.dir.m31 = 0.0;
    ybd->bact.dir.m32 = ybd->bact.Viewer.dir.m32 = 0.0;
    ybd->bact.dir.m33 = ybd->bact.Viewer.dir.m33 = 1.0;

    ybd->bact.dof.x = 0.0;
    ybd->bact.dof.y = 0.0;
    ybd->bact.dof.z = 0.0;
    ybd->bact.dof.v = 0.0;

    ybd->bact.tar_vec.x = 0.0;
    ybd->bact.tar_vec.y = 0.0;
    ybd->bact.tar_vec.z = 0.0;

    /*** (I)-Attribute auswerten ***/
    if (!yb_initAttrs(newo,ybd,attrs)) {
        _methoda(newo, OM_DISPOSE, NULL);
        return(NULL);
    };

    /*** Kommunikation mit Welt-Object ***/
    //if (ybd->world) {
    //    _method(ybd->world, OM_GET,
    //            YWA_MapSizeX,     &(ybd->map_size_x),
    //            YWA_MapSizeY,     &(ybd->map_size_y),
    //            TAG_DONE);
    //
    //    gsi_msg.abspos_x = ybd->bact.pos.x;
    //    gsi_msg.abspos_z = ybd->bact.pos.z;
    //    if (_methoda(ybd->world, YWM_GETSECTORINFO, &gsi_msg)) {
    //
    //        ybd->bact.SectX    = gsi_msg.sec_x;
    //        ybd->bact.SectY    = gsi_msg.sec_y;
    //        ybd->bact.Sector   = gsi_msg.sector;
    //        ybd->bact.relpos.x = gsi_msg.relpos_x;
    //        ybd->bact.relpos.z = gsi_msg.relpos_z;
    //
    //        /*** Bacterium in Bact-Liste des Sektors hängen ***/
    //        _AddTail((struct List *) &(ybd->bact.Sector->BactList),
    //                 (struct Node *) &(ybd->bact.SectorNode));
    //    } else {
    //
    //        /*** Initialisierungs-Position war ungültig! ***/
    //        _methoda(newo, OM_DISPOSE, NULL);
    //        return(NULL);
    //    };
    //} else {
    //    _methoda(newo, OM_DISPOSE, NULL);
    //    return(NULL);
    //};

    /*** TForm updaten ***/
    ybd->bact.tf.loc   = ybd->bact.pos;
    ybd->bact.tf.loc_m = ybd->bact.dir;

    /*** Status normal ***/
    ybd->bact.MainState = ACTION_NORMAL;
    
    /*** Die Levelgröße ***/
    _get( ybd->world, YWA_MapSizeX, &MX );
    _get( ybd->world, YWA_MapSizeY, &MY );
    ybd->bact.WorldX =  ((FLOAT) MX ) * SECTOR_SIZE;
    ybd->bact.WorldZ = -((FLOAT) MY ) * SECTOR_SIZE;
    ybd->bact.WSecX  = MX;
    ybd->bact.WSecY  = MY;

    /* --------------------------------------------------------
    ** Handle holen (und Liszt, und Schubert, und Bach...) 
    ** Das müssen wir leider hier machen (und immer wieder....)
    ** weil wir erst hier den Weltpointer haben
    ** ------------------------------------------------------*/
    _get( ybd->world, YWA_LocaleHandle, &BactLocaleHandle );

    /*** Ende ***/
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, yb_OM_DISPOSE, void *ignored)
/*
**  FUNCTION
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      09-Jun-95   floh    created
**      11-Jun-95   floh    neuer (sicherer) Weg, sich aus der
**                          Backterien-Liste des Sektors zu removen
*/
{
    struct ypabact_data *ybd;
    struct MinList *ls;
    struct MinNode *nd, *nx;
    struct Bacterium *me;

    ybd = INST_DATA(cl,o);
    me  = &(ybd->bact);

    /*** 22.00 Uhr, Nachtruhe! ***/
    _KillSoundCarrier( &(ybd->bact.sc) );

//    /*** mich von meinen Angreifern 'verabschieden' ***/
//    ls = &(ybd->attck_list);
//    nd = ls->mlh_Head;
//    while( nd->mln_Succ ) {
//
//        struct Bacterium *atr = ((struct OBNode *)nd)->bact;
//        if (me == atr->PrimaryTarget.Bact) {
//                atr->PrimTargetType     = TARTYPE_NONE;
//                atr->PrimaryTarget.Bact = NULL;
//        } else if (me == atr->SecondaryTarget.Bact) {
//                atr->SecTargetType        = TARTYPE_NONE;
//                atr->SecondaryTarget.Bact = NULL;
//        };
//
//        /*** Nachfolger vorher merken ***/
//        nx = nd->mln_Succ;
//
//        /*** den Angreifer aus meiner Attacker-List removen ***/
//        _Remove((struct Node *) nd);
//
//        /*** Next one ***/
//        nd = nx;
//    };

    /*** den umstaendlichen, aber sicheren DIE-Aufruf ***/
    ybd->bact.ExtraState & EXTRA_CLEANUP;
    if( !(ybd->bact.ExtraState & EXTRA_LOGICDEATH))
        _methoda( o, YBM_DIE, NULL );

    /*** aus Sektor-Bakterien-Liste entfernen (safe) ***/
    if (ybd->bact.Sector) {
        _Remove((struct Node *) &(ybd->bact.SectorNode));
    };

    /*** aus slave_list meines Masters (auch Welt-Object!) entfernen ***/
    if (ybd->bact.master != NULL) {
        _Remove((struct Node *) &(ybd->bact.slave_node.nd));
    };

    /*** OM_DISPOSE an alle Untergegebenen weitergeben ***/
    while (ybd->bact.slave_list.mlh_Head->mln_Succ) {
        Object *slave = ((struct OBNode *)ybd->bact.slave_list.mlh_Head)->o;
        _dispose(slave);
    };

    /*** Alle meine Waffen freigeben ***/
    while( ybd->bact.auto_list.mlh_Head->mln_Succ ) {
        
        struct OBNode *weapon = (struct OBNode *) ybd->bact.auto_list.mlh_Head;
        _Remove( (struct Node *) weapon );
        _dispose( weapon->o );
        }

    /*** OM_DISPOSE nach oben geben ***/
    return((BOOL)_supermethoda(cl,o,OM_DISPOSE,ignored));
}

/*-----------------------------------------------------------------*/
_dispatcher(void, yb_OM_SET, struct TagItem *attrs)
/*
**  FUNCTION
**
**  INPUTS
**
**  RESULTS
**      ---
**
**  CHANGED
**      09-Jun-95   floh    created
*/
{
    
    /* und die Superclass auch... */
    _supermethoda(cl,o,OM_SET,(Msg *)attrs);
    
    /* Attribute setzen */
    yb_setAttrs(o,INST_DATA(cl,o),attrs);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, yb_OM_GET, struct TagItem *attrs)
/*
**  FUNCTION
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      09-Jun-95   floh    created
*/
{
    /* Methode an Superclass */
    _supermethoda(cl,o,OM_GET,(Msg *)attrs);

    /* Attribute getten */
    yb_getAttrs(o,INST_DATA(cl,o),attrs);
}








