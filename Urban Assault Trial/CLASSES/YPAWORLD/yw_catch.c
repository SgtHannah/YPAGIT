/*
**  $Source$
**  $Revision$
**  $Date$
**  $Locker$
**  $Author$
**
**  yw_catch.c -- Event-Catcher fuer Tutorial-Levels.  
**
**  (C) Copyright 1998 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "nucleus/nucleus2.h"
#include "ypa/ypacatch.h"
#include "ypa/guimap.h"
#include "ypa/ypaworldclass.h"
#include "ypa/guiconfirm.h"
#include "ypa/guiabort.h"

extern struct YPAAbortReq AMR;
extern struct YPAMapReq MR;

/*-------------------------------------------------------------------
**  interne Prototypes
*/
void yw_AddEvent(struct ypaworld_data *ywd,ULONG event_code,LONG rel_next_item);

/*-----------------------------------------------------------------**
**                                                                 **
**  Event Checker Routinen                                         **
**                                                                 **
**-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
BOOL yw_CheckEnemiesDestroyed(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Falls es ausser dem User-Robo noch irgendwelche anderen
**      Robos gibt, kommt die Routine FALSE zurueck, sonst
**      TRUE.
**
**  CHANGED
**      18-Apr-98   floh    created
*/
{
    struct MinList *ls = &(ywd->CmdList);
    struct MinNode *nd;
    for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {
        struct Bacterium *b = ((struct OBNode *)nd)->bact;
        if ((b!=ywd->URBact) && 
            (b->BactClassID==BCLID_YPAROBO) &&
            (b->MainState != ACTION_DEAD)) 
        {
            return(FALSE);
        };
    };
    return(TRUE);
}

/*-----------------------------------------------------------------*/
BOOL yw_CheckCreateVehicle(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Wenn User in Robo oder Flak sitzt und keine Fahrzeuge
**      besitzt, und noch Feinfahrzeuge existieren, 
**      kommt die Routine FALSE zurueck, ansonsten TRUE.  
**
**  CHANGED
**      18-Apr-98   floh    created
*/
{
    if ((!yw_CheckEnemiesDestroyed(ywd)) &&
        ((ywd->UVBact==ywd->URBact)||(ywd->UserSitsInRoboFlak)) &&
        (ywd->NumCmdrs==0)) 
    {
        return(FALSE);
    } else {
        return(TRUE);
    };
}

/*-----------------------------------------------------------------*/
BOOL yw_CheckControlVehicle(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Wenn User in Robo oder Flak sitzt, und Fahrzeuge besitzt, 
**      und noch Feindfahrzeuge existieren, kommt die Routine
**      FALSE zurueck, sonst TRUE. 
**
**  CHANGED
**      18-Apr-98   floh    created
*/
{
    if ((!yw_CheckEnemiesDestroyed(ywd)) &&
        ((ywd->UVBact==ywd->URBact)||(ywd->UserSitsInRoboFlak)) &&
        (ywd->NumCmdrs>0))
    {
        return(FALSE);
    } else {
        return(TRUE);
    }; 
}

/*-----------------------------------------------------------------*/
BOOL yw_CheckMapComingUp(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Wenn User in Robo sitzt und die Map nicht offen ist,
**      kommt die Routine FALSE zurueck, sonst TRUE.
**
**  CHANGED
**      18-Apr-98   floh    created
*/
{
    if ((!yw_CheckEnemiesDestroyed(ywd)) &&
        ((ywd->UVBact==ywd->URBact)||(ywd->UserSitsInRoboFlak)) &&
        (MR.req.flags & REQF_Closed))
    {
        return(FALSE);
    } else {
        return(TRUE);
    };
}

/*-----------------------------------------------------------------*/
BOOL yw_CheckPowerStation(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Returniert TRUE, falls der User mindestens ein
**      Kraftwerk besitzt, sonst FALSE. 
**
**  CHANGED
**      18-Apr-98   floh    created
*/
{
    ULONG i;
    struct KraftWerk *kw_array = ywd->KraftWerks;
    for (i=0; i<ywd->FirstFreeKraftWerk; i++) {
        if (kw_array[i].sector &&
            (kw_array[i].sector->Owner == ywd->URBact->Owner))
        {
            return(TRUE);
        };
    };
    return(FALSE);
}

/*-----------------------------------------------------------------*/
BOOL yw_CheckSitsOnPowerStation(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Returniert TRUE, falls User-Station auf einem
**      Kraftwerk sitzt.
**
**  CHANGED
**      01-Jun-98   floh    created
*/
{
    if (ywd->URBact->Sector->WType == WTYPE_Kraftwerk) return(TRUE);
    else                                               return(FALSE);
}

/*-----------------------------------------------------------------*/
BOOL yw_CheckKeySectors(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Returniert TRUE, falls mindestens ein Beamgate
**      offen ist.
**
**  CHANGED
**      18-Apr-98   floh    created
*/
{
    /*** fuer alle BeamGates... ***/
    ULONG i;
    for (i=0; i<ywd->Level->NumGates; i++) {
        struct Gate *gate = &(ywd->Level->Gate[i]);
        if (gate->sec->WType == WTYPE_OpenedGate) {
            return(TRUE);
        };
    };            
    return(FALSE);
}

/*-----------------------------------------------------------------*/
BOOL yw_CheckBeamGate(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Returniert TRUE, wenn sich Hoststation in einem
**      offenen Beamgate befindet, sonst FALSE.
**
**  CHANGED
**      18-Apr-98   floh    created
*/
{
    if (ywd->URBact->Sector->WType == WTYPE_OpenedGate) return(TRUE);
    else                                                return(FALSE); 
}

/*=================================================================**
**                                                                 **
**  Hauptroutinen                                                  **
**                                                                 ** 
**=================================================================*/

/*-----------------------------------------------------------------*/
void yw_KillEventCatcher(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Deinitialisiert das Eventcatcher-Modul.
**
**  CHANGED
**      17-Apr-98   floh    created
*/
{
    if (ywd->EventCatcher) {
        _FreeVec(ywd->EventCatcher);
        ywd->EventCatcher = NULL;
    };
}

/*-----------------------------------------------------------------*/
BOOL yw_InitEventCatcher(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Initialisiert das EventCatcher-Modul.
**
**  CHANGED
**      17-Apr-98   floh    created
*/
{
    ywd->EventCatcher = _AllocVec(sizeof(struct ypa_EventCatcher),
                                  MEMF_PUBLIC|MEMF_CLEAR);
    if (ywd->EventCatcher) {
        return(TRUE);
    };
    return(FALSE);
}

/*-----------------------------------------------------------------*/
void yw_AddFirstHitEvent(struct ypaworld_data *ywd, 
                         BOOL (*event_func)(struct ypaworld_data *),
                         ULONG delay)
/*
**  CHANGED
**      18-Apr-98   floh    created
*/
{
    struct ypa_EventCatcher *ec = ywd->EventCatcher;
    struct ypa_EventLoopItem *eli = &(ec->events[ec->num_events++]);
    memset(eli,0,sizeof(struct ypa_EventLoopItem));
    eli->type        = YPAEVENTTYPE_FIRSTHIT;
    eli->delay       = delay;
    eli->event_func  = event_func;
}

/*-----------------------------------------------------------------*/
void yw_AddDelayedFirstHitEvent(struct ypaworld_data *ywd, 
                                BOOL (*event_func)(struct ypaworld_data *),
                                ULONG delay)
/*
**  CHANGED
**      18-Apr-98   floh    created
*/
{
    struct ypa_EventCatcher *ec = ywd->EventCatcher;
    struct ypa_EventLoopItem *eli = &(ec->events[ec->num_events++]);
    memset(eli,0,sizeof(struct ypa_EventLoopItem));
    eli->type        = YPAEVENTTYPE_DELAYED_FIRSTHIT;
    eli->delay       = delay;
    eli->event_func  = event_func;
}

/*-----------------------------------------------------------------*/
void yw_AddDelayedCyclicEvent(struct ypaworld_data *ywd, 
                              BOOL (*event_func)(struct ypaworld_data *),
                              ULONG delay)
/*
**  CHANGED
**      18-Apr-98   floh    created
*/
{
    struct ypa_EventCatcher *ec = ywd->EventCatcher;
    struct ypa_EventLoopItem *eli = &(ec->events[ec->num_events++]);
    memset(eli,0,sizeof(struct ypa_EventLoopItem));
    eli->type        = YPAEVENTTYPE_DELAYED_CYCLIC;
    eli->delay       = delay;
    eli->event_func  = event_func;
}

/*-----------------------------------------------------------------*/
void yw_AddCyclicEvent(struct ypaworld_data *ywd, 
                       BOOL (*event_func)(struct ypaworld_data *),
                       ULONG delay)
/*
**  CHANGED
**      
*/
{
    struct ypa_EventCatcher *ec = ywd->EventCatcher;
    struct ypa_EventLoopItem *eli = &(ec->events[ec->num_events++]);
    memset(eli,0,sizeof(struct ypa_EventLoopItem));
    eli->type        = YPAEVENTTYPE_CYCLIC;
    eli->delay       = delay;
    eli->event_func  = event_func;
}

/*-----------------------------------------------------------------*/
void yw_AddComplexEvent(struct ypaworld_data *ywd, 
                        BOOL (*event_func)(struct ypaworld_data *),
                        ULONG delay) 
/*
**  CHANGED
**      18-Apr-98   floh    created
*/
{
    struct ypa_EventCatcher *ec = ywd->EventCatcher;
    struct ypa_EventLoopItem *eli = &(ec->events[ec->num_events++]);
    memset(eli,0,sizeof(struct ypa_EventLoopItem));
    eli->type        = YPAEVENTTYPE_COMPLEX;
    eli->event_func  = event_func;
    eli->delay       = delay;
    eli->event_func  = event_func;
}

/*-----------------------------------------------------------------*/
void yw_AddTerminateEvent(struct ypaworld_data *ywd,
                          BOOL (*event_func)(struct ypaworld_data *),
                          ULONG delay)
/*
**  CHANGED
**      01-Jun-98   floh    created
*/ 
{
    struct ypa_EventCatcher *ec = ywd->EventCatcher;
    struct ypa_EventLoopItem *eli = &(ec->events[ec->num_events++]);
    memset(eli,0,sizeof(struct ypa_EventLoopItem));
    eli->type       = YPAEVENTTYPE_TERMINATE;
    eli->event_func = event_func;
    eli->delay      = delay;
    eli->event_func = event_func;
}

/*-----------------------------------------------------------------*/
void yw_AddLogMsg(struct ypaworld_data *ywd, ULONG lm_code)
/*
**  CHANGED
**      19-Apr-98   floh    created
*/
{
    struct ypa_EventCatcher *ec = ywd->EventCatcher;
    LONG event_num = ec->num_events-1;
    if (event_num >= 0) {
        struct ypa_EventLoopItem *eli = &(ec->events[event_num]);
        eli->act_logmsg = 0;
        eli->logmsg[eli->num_logmsg++] = lm_code;
    };
}

/*-----------------------------------------------------------------*/
void yw_SetEventLoop(struct ypaworld_data *ywd, ULONG loop_id) 
/*
**  FUNCTION
**      Initialisiert eine vordefinierte Eventloop, also
**      eine festgelegte Abfolge von Ereignissen und
**      Messages.
**
**  CHANGED
**      17-Apr-98   floh    created
*/
{
    struct ypa_EventCatcher *ec = ywd->EventCatcher;
    ec->event_loop_id           = loop_id;
    ec->last_msg_event          = -1;
    ec->last_msg_timestamp      = 0;
    ec->last_check_timestamp    = 0;
    ec->num_events = 0;
    switch (loop_id) {
        case EVENTLOOP_TUTORIAL1:
            yw_AddDelayedFirstHitEvent(ywd,NULL,5000);
                yw_AddLogMsg(ywd,LOGMSG_NOP);
            yw_AddComplexEvent(ywd,yw_CheckCreateVehicle,20000);
                yw_AddLogMsg(ywd,LOGMSG_EVENTMSG_CREATE);
            yw_AddComplexEvent(ywd,yw_CheckControlVehicle,20000);
                yw_AddLogMsg(ywd,LOGMSG_EVENTMSG_CONTROL);
            yw_AddComplexEvent(ywd,yw_CheckEnemiesDestroyed,20000);
                yw_AddLogMsg(ywd,LOGMSG_EVENTMSG_DESTROYROBO);
            yw_AddTerminateEvent(ywd,NULL,15000);
                yw_AddLogMsg(ywd,LOGMSG_EVENTMSG_COMPLETE_1);
            break;      
        
        case EVENTLOOP_TUTORIAL2:
            yw_AddDelayedFirstHitEvent(ywd,NULL,5000);
                yw_AddLogMsg(ywd,LOGMSG_NOP);
            yw_AddComplexEvent(ywd,yw_CheckCreateVehicle,20000);
                yw_AddLogMsg(ywd,LOGMSG_EVENTMSG_CREATE);
            yw_AddComplexEvent(ywd,yw_CheckMapComingUp,20000);
                yw_AddLogMsg(ywd,LOGMSG_EVENTMSG_MAP);
            yw_AddDelayedCyclicEvent(ywd,yw_CheckEnemiesDestroyed,20000);
                yw_AddLogMsg(ywd,LOGMSG_EVENTMSG_COMMAND);
                yw_AddLogMsg(ywd,LOGMSG_EVENTMSG_OPT_CONTROL);
                yw_AddLogMsg(ywd,LOGMSG_EVENTMSG_OPT_AGGR);
            yw_AddComplexEvent(ywd,yw_CheckEnemiesDestroyed,20000);
                yw_AddLogMsg(ywd,LOGMSG_EVENTMSG_DESTROYALL);
            yw_AddTerminateEvent(ywd,NULL,15000);
                yw_AddLogMsg(ywd,LOGMSG_EVENTMSG_COMPLETE_2);
            break;
        
        case EVENTLOOP_TUTORIAL3:
            yw_AddDelayedFirstHitEvent(ywd,NULL,3000);
                yw_AddLogMsg(ywd,LOGMSG_NOP);
            yw_AddDelayedFirstHitEvent(ywd,NULL,8000);
                yw_AddLogMsg(ywd,LOGMSG_EVENTMSG_WELCOME_3);
            yw_AddComplexEvent(ywd,yw_CheckPowerStation,60000);
                yw_AddLogMsg(ywd,LOGMSG_EVENTMSG_POWERSTATION);
                yw_AddLogMsg(ywd,LOGMSG_EVENTMSG_PSLIGHTINGSYM);
            yw_AddDelayedFirstHitEvent(ywd,NULL,3000);
                yw_AddLogMsg(ywd,LOGMSG_EVENTMSG_PSCONQUERED);
            yw_AddComplexEvent(ywd,yw_CheckSitsOnPowerStation,60000);
                yw_AddLogMsg(ywd,LOGMSG_EVENTMSG_TOTELEPORT);
                yw_AddLogMsg(ywd,LOGMSG_EVENTMSG_TELEPORT);
            yw_AddDelayedFirstHitEvent(ywd,NULL,3000);
                yw_AddLogMsg(ywd,LOGMSG_EVENTMSG_ABSORBINPROGRESS);
            yw_AddDelayedFirstHitEvent(ywd,NULL,9000);
                yw_AddLogMsg(ywd,LOGMSG_EVENTMSG_INCOMINGHS);
            yw_AddDelayedFirstHitEvent(ywd,NULL,5000);
                yw_AddLogMsg(ywd,LOGMSG_NOP);
            yw_AddComplexEvent(ywd,yw_CheckEnemiesDestroyed,60000);
                yw_AddLogMsg(ywd,LOGMSG_EVENTMSG_RESNEEDSYOU);
                yw_AddLogMsg(ywd,LOGMSG_EVENTMSG_ELIMINATEENEMY);
            yw_AddDelayedFirstHitEvent(ywd,NULL,4000);
                yw_AddLogMsg(ywd,LOGMSG_EVENTMSG_HSDESTROYED);
            yw_AddComplexEvent(ywd,yw_CheckKeySectors,60000);
                yw_AddLogMsg(ywd,LOGMSG_EVENTMSG_KEYSECTORS);
            yw_AddDelayedFirstHitEvent(ywd,NULL,3000);
                yw_AddLogMsg(ywd,LOGMSG_EVENTMSG_BEAMGATEOPEN);
            yw_AddDelayedFirstHitEvent(ywd,NULL,45000);
                yw_AddLogMsg(ywd,LOGMSG_NOP);
            yw_AddCyclicEvent(ywd,NULL,60000);
                yw_AddLogMsg(ywd,LOGMSG_EVENTMSG_BEAMOUTHS);
                yw_AddLogMsg(ywd,LOGMSG_EVENTMSG_BEAMGATE);
                yw_AddLogMsg(ywd,LOGMSG_EVENTMSG_HOWTOBEAM);
            break;
    };
}

/*-----------------------------------------------------------------*/
ULONG yw_CheckEvent(struct ypaworld_data *ywd, ULONG event_num)
/*
**  FUNCTION
**      Testet, ob das angegebene Event erfuellt ist oder nicht.
**
**  RESULT
**      returniert eins von YPAEVENTRES_BLOCK,
**      YPAEVENTRES_MSG, YPAEVENTRES_SKIP.
**
**  CHANGED
**      18-Apr-98   floh    created
*/
{
    struct ypa_EventCatcher *ec = ywd->EventCatcher;
    struct ypa_EventLoopItem *eli = &(ec->events[event_num]);
    ULONG retval;
    
    /*** falls Timestamp noch nicht initialisiert, dies jetzt tun ***/
    if (eli->timestamp == 0) eli->timestamp = ywd->TimeStamp;
    
    /*** je nach Event-Type abhandeln ***/    
    if (eli->type == YPAEVENTTYPE_FIRSTHIT) {
    
        if (!eli->first_hit) {
            eli->first_hit = TRUE;
            if (eli->event_func) {
                if (eli->event_func(ywd)) retval = YPAEVENTRES_SKIP;
                else                      retval = YPAEVENTRES_MSG;
            } else retval = YPAEVENTRES_MSG;
        } else {
            retval = YPAEVENTRES_SKIP;
        };
    
    } else if (eli->type == YPAEVENTTYPE_DELAYED_FIRSTHIT) {
        
        LONG td = ywd->TimeStamp - eli->timestamp;
        if (td > eli->delay) {
            if (!eli->first_hit) {
                eli->first_hit = TRUE;
                if (eli->event_func) {
                    if (eli->event_func(ywd)) retval = YPAEVENTRES_SKIP;
                    else                      retval = YPAEVENTRES_MSG;
                } else retval = YPAEVENTRES_MSG;
            } else {
                retval = YPAEVENTRES_SKIP;
            };
        } else {
            retval = YPAEVENTRES_BLOCK;
        };
                    
    } else if (eli->type == YPAEVENTTYPE_DELAYED_CYCLIC) {
        
        LONG td = ywd->TimeStamp - eli->timestamp;
        if (td > eli->delay) {
            eli->timestamp = ywd->TimeStamp;
            if (eli->event_func) {
                if (eli->event_func(ywd)) retval = YPAEVENTRES_SKIP;
                else                      retval = YPAEVENTRES_MSG;
            } else retval = YPAEVENTRES_MSG;
        } else {
            retval = YPAEVENTRES_SKIP;
        };
    
    } else if (eli->type == YPAEVENTTYPE_CYCLIC) {
        
        LONG td = ywd->TimeStamp - eli->timestamp;
        if (!eli->first_hit) {
            eli->first_hit = TRUE;
            if (eli->event_func) {
                if (eli->event_func(ywd)) retval = YPAEVENTRES_SKIP;
                else                      retval = YPAEVENTRES_MSG;
            } else retval = YPAEVENTRES_MSG;
        } else if (td > eli->delay) {
            eli->timestamp = ywd->TimeStamp;
            if (eli->event_func) {
                if (eli->event_func(ywd)) retval = YPAEVENTRES_SKIP;
                else                      retval = YPAEVENTRES_MSG;
            } else retval = YPAEVENTRES_MSG;
        } else {
            retval = YPAEVENTRES_SKIP;
        };
    
    } else if (eli->type == YPAEVENTTYPE_COMPLEX) {
        /*** Event-Status wird von Eventfunc ermittelt ***/
        if (eli->event_func) {
            if (eli->event_func(ywd)) retval = YPAEVENTRES_SKIP;
            else                      retval = YPAEVENTRES_MSG;
        } else retval = YPAEVENTRES_SKIP;
        
    } else if (eli->type == YPAEVENTTYPE_TERMINATE) {

        LONG td = ywd->TimeStamp - eli->timestamp;
        if (td > eli->delay) {
            if (!eli->first_hit) {
                eli->first_hit = TRUE;
                if (eli->event_func) {
                    if (eli->event_func(ywd)) retval = YPAEVENTRES_SKIP;
                    else                      retval = YPAEVENTRES_MSG;
                } else retval = YPAEVENTRES_MSG;
                if (yw_CRGetStatus(ywd) != YPACR_STATUS_OPEN) {
                    AMR.action = AMR_BTN_CANCEL;
                    yw_OpenCR(ywd,ypa_GetStr(ywd->LocHandle,STR_CONFIRM_TUTORIALEXIT,"2470 == EXIT TUTORIAL MISSION ?"),&AMR);
                };
            } else {
                retval = YPAEVENTRES_SKIP;
            };
        } else {
            retval = YPAEVENTRES_BLOCK;
        };
        
    } else {
        retval = TRUE;
    };
    return(retval);
}

/*-----------------------------------------------------------------*/
void yw_PutEventMessage(struct ypaworld_data *ywd, LONG event_num)
/*
**  CHANGED
**      18-Apr-98   floh    created
*/
{
    struct logmsg_msg lm;
    struct ypa_EventCatcher *ec = ywd->EventCatcher;
    struct ypa_EventLoopItem *eli = &(ec->events[event_num]);
    UBYTE *str = NULL;
    ec->last_msg_event     = event_num;
    ec->last_msg_timestamp = ywd->TimeStamp;
    lm.bact = NULL;
    lm.pri  = 100;
    lm.code = eli->logmsg[eli->act_logmsg++];
    if (eli->act_logmsg >= eli->num_logmsg) eli->act_logmsg=0;
    lm.msg = NULL;
    _methoda(ywd->world,YWM_LOGMSG,&lm);
}

/*-----------------------------------------------------------------*/
void yw_TriggerEventCatcher(struct ypaworld_data *ywd)
/*
**  CHANGED
**      18-Apr-98   floh    created
*/
{
    struct ypa_EventCatcher *ec = ywd->EventCatcher;
    if (ec->event_loop_id != EVENTLOOP_NONE) {
        LONG event_num  = 0;
        ULONG event_res = YPAEVENTRES_SKIP;
        
        /*** alle bereits erfuellten Events skippen ***/        
        while ((event_num < ec->num_events) && 
               ((event_res = yw_CheckEvent(ywd,event_num)) == YPAEVENTRES_SKIP)) 
        {
            event_num++;
        };
        if (event_res == YPAEVENTRES_MSG) {
            if (event_num < ec->num_events) {        
                /*** event_num jetzt auf 1.nicht erfuellten Event ***/
                if (event_num != ec->last_msg_event) {
                    /*** das ist eine neue Message, sofort ausgeben ***/
                    yw_PutEventMessage(ywd,event_num);
                } else {
                    /*** sonst die Message verzoegert cyclen ***/
                    LONG msg_dt = ywd->TimeStamp - ec->last_msg_timestamp;
                    if (msg_dt > ec->events[event_num].delay) {
                        yw_PutEventMessage(ywd,event_num);
                    };
                };
            };
        };
    };
}
