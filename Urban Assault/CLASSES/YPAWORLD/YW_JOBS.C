/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_jobs.c,v $
**  $Revision: 38.5 $
**  $Date: 1998/01/06 16:22:07 $
**  $Locker:  $
**  $Author: floh $
**
**  Primitives Jobhandling für Welt-Klasse. Jeder Robo hat
**  Anspruch auf einen "Timed Job", der einmal pro Frame
**  aktualisiert wird.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include "nucleus/nucleus2.h"
#include "engine/engine.h"
#include "ypa/ypaworldclass.h"

#include "string.h"

#include "yw_protos.h"

_extern_use_nucleus
_extern_use_audio_engine

/*-----------------------------------------------------------------*/
void yw_ResetBuildJobModule(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Initialisiert das Jobs-Array in der LID. Die
**      Job-Entries im Cell-Area werden nicht initialisiert,
**      weil deren "Leerzustand" 0 ist, und am Anfang jedes
**      Levels wird das Cell-Area eh gelöscht.
**
**      yw_ResetJobModule() sollte ausschließlich aus
**      YWM_NEWLEVEL() ausgeführt werden.
**
**  INPUTS
**      ywd     - Ptr auf LID des Welt-Objects
**
**  RESULTS
**      ---
**
**  CHANGED
**      29-Jan-96   floh    created
*/
{
    /*** Radikale Ausradierung ***/
    memset(ywd->BuildJobs,0,sizeof(ywd->BuildJobs));
}

/*-----------------------------------------------------------------*/
BOOL yw_LockBuildJob(struct ypaworld_data *ywd,
                     ULONG id,
                     ULONG duration,
                     LONG sec_x, LONG sec_y,
                     ULONG owner, ULONG bp)
/*
**  FUNCTION
**      Lockt BuildJob[id], so er nicht bereits gelockt ist.
**      Falls der Lock gelingt, wird er initialisiert.
**
**      Wenn der Lock abgelaufen ist, wird mit einem yw_SetSector()
**      das Gebäude gebaut.
**
**  INPUTS
**      ywd      - LID des Welt-Objects
**      id       - Lock-ID [1..7]
**      duration - Zeitdauer des Jobs in millisec
**      sec_x    - X-Koord des Sektors, auf den der Job wirkt
**      sec_y    - ditto
**      owner    - Späterer Besitzer des Bauauftrages
**      bp       - Buildproto-Nummer
**
**  RESULTS
**      TRUE    - Job ist gelockt und initialisiert
**      FALSE   - Job war bereits vergeben
**
**  CHANGED
**      29-Jan-96   floh    created
**      17-May-96   floh    BuildJobs auf Randsektoren werden
**                          jetzt ignoriert.
**      31-Oct-96   floh    + BuildFX-Handling
*/
{
    if (id < MAXNUM_JOBS) {
        if (!ywd->BuildJobs[id].lock) {

            struct Cell *sec;
            struct MinList *ls;
            struct MinNode *nd;

            /*** Randsektoren ignorieren ***/
            if ((sec_x == 0) || (sec_y == 0) ||
                (sec_x == (ywd->MapSizeX-1)) ||
                (sec_y == (ywd->MapSizeY-1)))
            {
                /*** ein RandSektor ***/
                return(FALSE);
            };

            /*** Job allokieren ***/
            ywd->BuildJobs[id].lock = TRUE;
            ywd->BuildJobs[id].age  = 0;
            ywd->BuildJobs[id].duration = duration;
            ywd->BuildJobs[id].sec_x    = sec_x;
            ywd->BuildJobs[id].sec_y    = sec_y;
            ywd->BuildJobs[id].owner    = owner;
            ywd->BuildJobs[id].bp       = bp;

            /*** den entsprechenden Sektor "markieren" ***/
            sec = &(ywd->CellArea[sec_y * ywd->MapSizeX + sec_x]);
            sec->WType  = WTYPE_JobLocked;
            sec->WIndex = id;

            /*** für FX muß ein "Originator-Robo" gefunden werden ***/
            ls = &(ywd->CmdList);
            for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {

                struct OBNode *obnd = (struct OBNode *)nd;
                struct Bacterium *rbact = obnd->bact;

                if ((BCLID_YPAROBO == rbact->BactClassID) &&
                    (owner == rbact->Owner))
                {
                    _StartSoundSource(&(rbact->sc),VP_NOISE_BUILD);
                    break;
                };
            };

            return(TRUE);
        };
    };

    return(FALSE);
}

/*-----------------------------------------------------------------*/
void yw_DoBuildJobs(Object *world, struct ypaworld_data *ywd, ULONG frame_time)
/*
**  FUNCTION
**      Addiert auf das Alter aller Jobs die <frame_time>,
**      Jobs die ihr Maximal-Alter überschritten haben,
**      werden freigegeben.
**
**  INPUTS
**      ywd         - Ptr auf LID des Welt-Objects
**      frame_time  - aktuelle Frametime
**
**  CHANGED
**      29-Jan-96   floh    created
**      26-Sep-97   floh    + VoiceOverMsg Support
*/
{
    ULONG i;
    for (i=0; i<MAXNUM_JOBS; i++) {
        if (ywd->BuildJobs[i].lock) {
            ywd->BuildJobs[i].age += frame_time;
            if (ywd->BuildJobs[i].age >= ywd->BuildJobs[i].duration) {

                /*** Job freigeben ***/
                ULONG bp_num = ywd->BuildJobs[i].bp;
                ULONG sec_x  = ywd->BuildJobs[i].sec_x;
                ULONG sec_y  = ywd->BuildJobs[i].sec_y;
                struct Cell *sec;

                ywd->BuildJobs[i].lock = FALSE;

                sec = &(ywd->CellArea[sec_y * ywd->MapSizeX + sec_x]);
                sec->WType  = 0;
                sec->WIndex = 0;

                /*** Gebäude hinsetzen ***/
                yw_SetSector(world,ywd,sec_x,sec_y,
                             ywd->BuildJobs[i].owner,
                             bp_num,
                             FALSE);

                /*** VoiceOver Meldung für eigene Bauwerke ***/
                if (ywd->BuildJobs[i].owner == ywd->URBact->Owner) {
                    if (ywd->BP_Array[bp_num].BaseType != BUILD_BASE_NONE) {
                        struct logmsg_msg lm;
                        lm.bact = ywd->URBact;
                        lm.msg  = NULL;
                        lm.pri  = 65;
                        switch(ywd->BP_Array[bp_num].BaseType) {
                            case BUILD_BASE_KRAFTWERK:
                                lm.code = LOGMSG_POWER_CREATED;
                                break;
                            case BUILD_BASE_RADARSTATION:
                                lm.code = LOGMSG_RADAR_CREATED;
                                break;
                            case BUILD_BASE_DEFCENTER:
                                lm.code = LOGMSG_FLAK_CREATED;
                                break;
                            default:
                                lm.code = LOGMSG_NOP;
                                break;
                        };
                        _methoda(world,YWM_LOGMSG,&lm);
                    };
                };
            };
        };
    };

    /*** Ende ***/
}

