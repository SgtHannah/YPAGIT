/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_sim.c,v $
**  $Revision: 38.35 $
**  $Date: 1998/01/06 16:27:09 $
**  $Locker: floh $
**  $Author: floh $
**
**  High-Level-Methoden für Abhandlung der 'Simulation'.
**
**  (C) Copyright 1995 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/lists.h>
#include <exec/nodes.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "engine/engine.h"
#include "visualstuff/ov_engine.h"
#include "audio/audioengine.h"

#include "input/idevclass.h"
#include "input/inputclass.h"
#include "bitmap/displayclass.h"
#include "ypa/ypabactclass.h"
#include "ypa/yparoboclass.h"
#include "ypa/guimap.h"
#include "ypa/guifinder.h"
#include "ypa/guiconfirm.h"
#include "ypa/ypaworldclass.h"

#include "bitmap/winddclass.h"
#include "input/winpclass.h"

#include "yw_protos.h"
#include "yw_gsprotos.h"
#include "yw_netprotos.h"

#include "network/networkclass.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_audio_engine
_extern_use_input_engine
_extern_use_ov_engine
_extern_use_tform_engine

extern struct YPAStatusReq SR;
extern struct YPAMapReq MR;
extern struct YPAFinder FR;
extern BOOL REDUCE_DATA_RATE;

/*** VORSICHT HACK: ypa_DoFrame() ist der FrameHandler aus ypa.c ***/
extern void ypa_DoFrame(void);
extern UBYTE  **GlobalLocaleHandle;

extern struct YPAListReq SubMenu;

/*-----------------------------------------------------------------*/
void yw_ClearFootPrints(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Löscht alle Footprints der Welt, bzw. setzt
**      sie, wenn es sich um einen Sektor mit
**      EFact > 0 (!) handelt.
**
**  CHANGED
**      21-Mar-96   floh    created
**      23-May-96   floh    Zumindest für den User ist immer alles
**                          sichtbar.
**      22-Aug-96   floh    Footprint wird jetzt auch gesetzt,
**                          wenn der Energie-Level des Sektors
**                          auf 0 steht.
**      30-Oct-96   floh    Experiment: alle Gegner sehen alle
**                          Kraftwerke, das sollte etwas mehr
**                          Action ins Spiel bringen.
*/
{
    ULONG num_sec    = ywd->MapSizeX * ywd->MapSizeY;
    struct Cell *sec = ywd->CellArea;
    ULONG i;

    /*** Owner-Footprints ***/
    for (i=0; i<num_sec; i++) {
        sec->FootPrint = (1<<sec->Owner);
        #ifdef YPA_DESIGNMODE
        sec->FootPrint = (1<<ywd->URBact->Owner);
        #endif
        #ifdef YPA_CUTTERMODE
        sec->FootPrint = (1<<ywd->URBact->Owner);
        #endif
        sec++;
    };
}

/*-----------------------------------------------------------------*/
void yw_InputControl(struct ypaworld_data *ywd, struct VFMInput *ip)
/*
**  FUNCTION
**      "Filter" für diverse Eingabe-Sachen, zum Beispiel
**      die zusätzliche Mouse-Vehikel-Kontrolle etc...
**
**  CHANGED
**      15-Jun-96   floh    created
**      20-Jun-96   floh    Umschalten des ControlLock jetzt per
**                          CIF_RMOUSEUP, damit es nicht mehr vorkommt,
**                          das man nach einer Map-Drag-Aktion ausversehen
**                          in den Kontroll-Lock schaltet.
**      27-Jun-96   floh    + Slider-Änderungen addieren sich auf
**                          + MMB wieder Bremse
**      23-Jul-96   floh    + MouseControl im Robo wird auf
**                            die Slider [6,7] umgeleitet (Cockpit-
**                            View)
**                          + Mousesteuerung generell bissel umgebaut
**      27-Jul-96   floh    + Panzer-Fadenkreuz etwas sensibler
**      10-Aug-96   floh    + Robo-Kanzel wird jetzt über die
**                            Standard-Slider bewegt
**      03-Nov-96   floh    + ControlLock generell aus, wenn UserFahrzeug
**                            tot.
**      14-May-97   floh    + Anpassungen für Joystick-Support
**      26-May-97   floh    + im Robo hat der Joystick keine Auswirkung
**                            mehr
**                          + Throttle sollte wieder funktionieren...
**                          + Joystick-Disable-Handling
**      27-May-97   floh    + Bugfix: Joystick-Throttle sollte jetzt wieder
**                            funken
**      01-Oct-97   floh    + Joystick wird deaktiviert, sobald eine
**                            Richtungs/Höhen/Speed-Taste gedrückt wird
**                            und aktiviert, sobald der Joystick bewegt wird.
**                          + Joystick für Robo wieder zugelassen
**      29-Oct-97   floh    + bei Fahrzeug-Wechsel wird Joystick-Blanker
**                            gesetzt, damit macht Joystick im Robo Sinn
**      05-Dec-97   floh    + rechter Mausklick wird nicht mehr ueber Statusreq
**                            akzeptiert, und nicht, wenn NEW,ADD,BUILD,BEAM oder
**                            CONTROL aktiviert ist.
**      15-May-98   floh    + Joystick-Handling umgeschrieben...
*/
{
    struct ClickInfo *ci = &(ip->ClickInfo);
    ULONG joy_disable = FALSE;

    /*** Joystick-Signal-Button weglöschen ***/
    ip->Buttons &= ~(1<<31);

    /*** Joystick disable? ***/
    if (ywd->Prefs.valid && (ywd->Prefs.Flags & YPA_PREFS_JOYDISABLE)) {
        joy_disable = TRUE;
    };

    if (ywd->UVBact->MainState == ACTION_DEAD){
        /*** User-Vehicle tot ***/
        ywd->ControlLock = FALSE;
    
    /*** Right Mouse Button Handling ***/
    } else if (ci->flags & CIF_RMOUSEDOWN) {

        if ((ywd->ControlLock == FALSE)       && 
            (ci->box != &(MR.req.req_cbox))   && 
            (ci->box != &(SR.req.req_cbox))   &&
            (ci->box != &(FR.l.Req.req_cbox)) &&
            ((SR.ActiveMode & STAT_MODEF_ORDER) || (ywd->ActCmdr == -1)))
        {
            /*** ControlLock ein, wenn RMB down ***/
            Object *input;
            struct inp_delegate_msg idm;
            struct idev_reset_msg irm;

            /*** Control-Lock aktivieren ***/
            ywd->ControlLock = TRUE;

            /*** Mouse-Slider resetten ***/
            _IE_GetAttrs(IET_Object, &input, TAG_DONE);
            irm.rtype  = IRTYPE_CENTER;
            idm.type   = ITYPE_SLIDER;
            idm.num    = 10;
            idm.method = IDEVM_RESET;
            idm.msg    = &irm;
            _methoda(input, IM_DELEGATE, &idm);
            idm.num    = 11;
            _methoda(input, IM_DELEGATE, &idm);
        } else if (ywd->ControlLock) {
            /*** ControlLock aus... ***/
            ywd->ControlLock = FALSE;
        } else if ((ci->box != &(MR.req.req_cbox)) && 
                   (ci->box != &(SR.req.req_cbox)) &&
                   (!(SR.ActiveMode & STAT_MODEF_ORDER)))
        {
            /*** New/Add/Build/Beam/Control mit rechten Mausklick deaktivieren ***/
            SR.ActiveMode = (STAT_MODEF_ORDER & SR.EnabledModes);
        };
    };

    /*** falls Submenu offen, up/down neutralisieren ***/
    if (!(SubMenu.Req.flags & REQF_Closed)) {
        ip->Slider[1] = 0.0;
    };

    /*** MouseControl-Handling ***/
    if (ywd->ControlLock) {

        /*** GUI-Zeugs neutralisieren ***/
        ci->flags &= ~(CIF_BUTTONDOWN|CIF_BUTTONHOLD|CIF_BUTTONUP);
        ci->btn    = -1;

        /*** Mouse-Slider auf "normale" Slider umleiten ***/
        ip->Slider[0] += ip->Slider[10]; // Flugrichtung
        ip->Slider[1] += ip->Slider[11]; // Flughöhe
        ip->Slider[3] += ip->Slider[10]; // Fahrrichtung
        ip->Slider[5] -= (ip->Slider[11]*2.5);  // -> Kanone hoch runter!!!
        ip->Slider[4] += ip->Slider[2];  // Fahrspeed-Hack

        if (ci->flags & CIF_MOUSEHOLD)  ip->Buttons |= (1<<0);  // Fire
        if (ci->flags & CIF_MMOUSEHOLD) ip->Buttons |= (1<<3);  // Brakes
    };

    /*** Joystick-Control-Handling ***/
    if (!joy_disable) {

        #define JOY_EQUAL(old,new) (fabs(old-new)<0.2)
        ULONG joy_moved;

        /*** Sonderfall 1.Frame, definierten Anfangszustand herstellen ***/
        if (ywd->FrameCount == 0) {
            ywd->PrevJoyX = ip->Slider[12];
            ywd->PrevJoyY = ip->Slider[13];
            ywd->PrevJoyZ = ip->Slider[14];
        };
        joy_moved = ((!JOY_EQUAL(ywd->PrevJoyX,ip->Slider[12]))||
                     (!JOY_EQUAL(ywd->PrevJoyY,ip->Slider[13]))||
                     (!JOY_EQUAL(ywd->PrevJoyZ,ip->Slider[14])));
        if (joy_moved) {
            ywd->PrevJoyX = ip->Slider[12];
            ywd->PrevJoyY = ip->Slider[13];
            ywd->PrevJoyZ = ip->Slider[14];
        };

        /*** Joystick deaktivieren? ***/
        if (ywd->DoJoystick) {
            /*** eine wichtige Taste gedrückt? ***/
            if (((ip->Slider[0] != 0.0)||
                 (ip->Slider[1] != 0.0)||
                 (ip->Slider[3] != 0.0)) &&
                 (!joy_moved))
            {
                ywd->DoJoystick = FALSE;
            };
        } else {
            /*** Joystick aktivieren? ***/
            if ((ip->Slider[0] == 0.0) &&
                (ip->Slider[1] == 0.0) &&
                (ip->Slider[3] == 0.0) &&
                (joy_moved))
            {
                ywd->DoJoystick = TRUE;
            };
        };

        if (ywd->DoJoystick) {
            ip->Slider[0] += ip->Slider[12];    // JoyX: Flugrichtung
            ip->Slider[1] += ip->Slider[13];    // JoyY: Flughoehe
            ip->Slider[3] += ip->Slider[12];    // JoyX: Fahrrichtung
            ip->Slider[4] += ip->Slider[14];    // Throttle: Speed
            ip->Slider[5] -= ip->Slider[13] * 2.5;    // JoyY: Kanone hoch runter

            /*** Joyenable Flag an ***/
            ip->Buttons |= (1<<31);

            /*** Bodenfahrzeug: Joystick zentriert, Bremse ***/
            if ((ywd->UVBact->BactClassID == BCLID_YPATANK) ||
                (ywd->UVBact->BactClassID == BCLID_YPACAR))
            {
                if (JOY_EQUAL(ip->Slider[14],0.0)) {    // Throttle
                    /*** Brakes ***/
                    ip->Buttons |= (1<<3);
                } else {
                    ip->Buttons &= ~(1<<3);
                };
            };

            /*** Luftfahrzeug-Beschleunigung per Throttle ***/
            if ((ywd->UVBact->BactClassID != BCLID_YPATANK) &&
                (ywd->UVBact->BactClassID != BCLID_YPACAR))
            {
                if ((ip->Slider[14] < -0.3) || (ip->Slider[14] > 0.3)) {
                    ip->Slider[4] += ip->Slider[14];
                    ip->Slider[2] += ip->Slider[14];
                };
            };
        };
        /*** Luftfahrzeug: Hatview beschleunigt ***/
        if ((ywd->UVBact->BactClassID != BCLID_YPATANK) &&
            (ywd->UVBact->BactClassID != BCLID_YPACAR))
        {
            ip->Slider[4] += ip->Slider[16];    // Beschl. per HatView
            ip->Slider[2] += ip->Slider[16];
        };

        if (ip->Buttons & (1<<16)) {
            /*** Fire ***/
            ip->Buttons |= (1<<0);
        };
        if (ip->Buttons & (1<<17)) {
            /*** MG ***/
            ip->Buttons |= (1<<2);
        };
        if (ip->Buttons & (1<<19)) {
            /*** Brakes ***/
            ip->Buttons |= (1<<3);
        };
    };
}

/*-----------------------------------------------------------------*/
void yw_GetRealViewerPos(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Ermittelt <ywd->ViewerPos> und <ywd->ViewerDir>
**      (die kann ja verschieden sein, je nachdem, ob es
**      ein ExtraViewer oder net ist).
**
**  CHANGED
**      21-Jul-96   floh    created
**      22-Jul-96   floh    aaargh, ich muß die RelPos ja mitrotieren!
*/
{
    ULONG is_extra_viewer;

    _get(ywd->Viewer->BactObject, YBA_ExtraViewer, &is_extra_viewer);
    if (is_extra_viewer) {
        struct flt_triple *ap = &(ywd->Viewer->pos);
        struct flt_triple *rp = &(ywd->Viewer->Viewer.relpos);
        struct flt_m3x3 *m    = &(ywd->Viewer->dir);
        ywd->ViewerPos.x = ap->x + m->m11*rp->x + m->m21*rp->y + m->m31*rp->z;
        ywd->ViewerPos.y = ap->y + m->m12*rp->x + m->m22*rp->y + m->m32*rp->z;
        ywd->ViewerPos.z = ap->z + m->m13*rp->x + m->m23*rp->y + m->m33*rp->z;
        ywd->ViewerDir = ywd->Viewer->Viewer.dir;
    } else {
        ywd->ViewerPos = ywd->Viewer->pos;
        ywd->ViewerDir = ywd->Viewer->dir;
    };
}

/*-----------------------------------------------------------------*/
void yw_CheckIfUserSitsInRoboFlak(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Wie der Name schon sagt...
**      ywd->UserSitsInRoboFlak wird auf TRUE/FALSE gesetzt
**
**  CHANGED
**      22-Jul-96   floh    created
*/
{
    ywd->UserSitsInRoboFlak = FALSE;
    if (ywd->UserRobo != ywd->UserVehicle) {
        struct gun_data *gd = NULL;
        _get(ywd->UserRobo, YRA_GunArray, &gd);
        if (gd) {
            ULONG i;
            for (i=0; i<NUMBER_OF_GUNS; i++) {
                if (ywd->UserVehicle == gd[i].go) ywd->UserSitsInRoboFlak=TRUE;
            };
        };
    };
}

/*-----------------------------------------------------------------*/
void yw_GetVehicleStats(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Sammelt Statistics über die Vehicle.
**
**  CHANGED
**      02-May-97   floh    created
*/
{
    ULONG i;
    struct MinList *ls;
    struct MinNode *nd;

    /*** Vehikel-Statistix sammeln ***/
    memset(&(ywd->NumSquads),0,sizeof(ywd->NumSquads));
    memset(&(ywd->NumVehicles),0,sizeof(ywd->NumVehicles));
    memset(&(ywd->NumFlaks),0,sizeof(ywd->NumFlaks));
    memset(&(ywd->NumRobos),0,sizeof(ywd->NumRobos));
    memset(&(ywd->NumWeapons),0,sizeof(ywd->NumWeapons));
    ywd->AllSquads   = 0;
    ywd->AllVehicles = 0;
    ywd->AllFlaks    = 0;
    ywd->AllWeapons  = 0;
    ywd->AllRobos    = 0;

    ls = &(ywd->CmdList);
    for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {

        /*** Robo-Ebene ***/
        struct Bacterium *b = ((struct OBNode *)nd)->bact;

        /*** Brocken sind "Owner 0 Robos" ***/
        ywd->NumRobos[b->Owner]++;

        /*** falls ein Robo, seine Slaves scannen ***/
        if (b->Owner != 0) {
            struct MinList *cmd_ls;
            struct MinNode *cmd_nd;
            cmd_ls = &(b->slave_list);
            for (cmd_nd=cmd_ls->mlh_Head;
                 cmd_nd->mln_Succ;
                 cmd_nd=cmd_nd->mln_Succ)
            {
                struct Bacterium *cmd = ((struct OBNode *)cmd_nd)->bact;
                struct MinList *slv_ls;
                struct MinNode *slv_nd;
                struct MinList *wpn_ls;
                struct MinNode *wpn_nd;
                ULONG is_flak_squad = FALSE;

                if (BCLID_YPAGUN == cmd->BactClassID) {
                    /*** eine Flak! ***/
                    is_flak_squad = TRUE;
                    ywd->NumFlaks[cmd->Owner]++;
                } else {
                    /*** ein "normaler" Commander ***/
                    ywd->NumSquads[cmd->Owner]++;
                    ywd->NumVehicles[cmd->Owner]++;
                };

                /*** Weapon-Liste des Commanders durchackern ***/
                wpn_ls = &(cmd->auto_list);
                for (wpn_nd=wpn_ls->mlh_Head;
                     wpn_nd->mln_Succ;
                     wpn_nd=wpn_nd->mln_Succ)
                {
                    ywd->NumWeapons[cmd->Owner]++;
                };

                /*** die Slaves durchackern ***/
                slv_ls = &(cmd->slave_list);
                for (slv_nd=slv_ls->mlh_Head;
                     slv_nd->mln_Succ;
                     slv_nd=slv_nd->mln_Succ)
                {
                    struct Bacterium *slv = ((struct OBNode *)slv_nd)->bact;
                    if (is_flak_squad) {
                        ywd->NumFlaks[slv->Owner]++;
                    } else {
                        ywd->NumVehicles[cmd->Owner]++;
                    };

                    /*** Weapon-Liste des Slaves durchackern ***/
                    wpn_ls = &(slv->auto_list);
                    for (wpn_nd=wpn_ls->mlh_Head;
                         wpn_nd->mln_Succ;
                         wpn_nd=wpn_nd->mln_Succ)
                    {
                        ywd->NumWeapons[cmd->Owner]++;
                    };
                };
            };
        };
    };

    /*** All- und Max-Variablen opdatern ***/
    for (i=0; i<MAXNUM_ROBOS; i++) {
        ywd->AllSquads   += ywd->NumSquads[i];
        ywd->AllVehicles += ywd->NumVehicles[i];
        ywd->AllFlaks    += ywd->NumFlaks[i];
        ywd->AllWeapons  += ywd->NumWeapons[i];
        ywd->AllRobos    += ywd->NumRobos[i];
    };
    if (ywd->MaxSquads   < ywd->AllSquads)   ywd->MaxSquads   = ywd->AllSquads;
    if (ywd->MaxVehicles < ywd->AllVehicles) ywd->MaxVehicles = ywd->AllVehicles;
    if (ywd->MaxFlaks    < ywd->AllFlaks)    ywd->MaxFlaks    = ywd->AllFlaks;
    if (ywd->MaxWeapons  < ywd->AllWeapons)  ywd->MaxWeapons  = ywd->AllWeapons;
    if (ywd->MaxRobos    < ywd->AllRobos)    ywd->MaxRobos    = ywd->AllRobos;
}

/*-----------------------------------------------------------------*/
UBYTE *yw_PutDbgMsg(struct ypaworld_data *ywd, UBYTE *str, UBYTE **buf, UBYTE *string, ...)
/*
**  FUNCTION
**      Druckt  per sprintf() eine Textmessage nach (*buf),
**      inkrementiert dabei buf um resultierende Länge
**      des Textes und generiert in str eine dbcs()
**      Anweisung um das ganze später per RASTM_Text
**      auszugeben. Die Routine erzeugt KEIN newline()
**
**  CHANGED
**      28-Feb-98   floh    created
*/
{
    UBYTE *buf_ptr = *buf;
    ULONG len;
    va_list arglist;
    va_start(arglist,string);
    len = vsprintf(buf_ptr,string,arglist);
    freeze_dbcs_pos(str);
    put_dbcs(str,ywd->DspXRes,DBCSF_LEFTALIGN,buf_ptr);
    buf_ptr += (len+1);
    *buf = buf_ptr;
    return(str);
}

/*-----------------------------------------------------------------*/
void yw_PutDebugInfo(struct ypaworld_data *ywd, struct VFMInput *ip)
/*
**  FUNCTION
**      Gibt ein paar Debug-Informationen aus (wurde
**      früher über die Debug-Requester abgewickelt).
**
**  CHANGED
**      08-Sep-96   floh    created
**      09-Apr-97   floh    Upd von AF
**      02-May-97   floh    + Vehikel-Anzahl-Statistix
**      11-Jun-97   floh    + Versions-String
**      28-Feb-98   floh    + sollte jetzt mit DBCS Strings arbeiten
**      14-May-98   floh    + neue Page fuer Input-Info
*/
{
    if (ywd->DebugInfo) {

        UBYTE buf[2048];
        UBYTE dbcs_buf[4096];
        struct rast_text rt;
        UBYTE *str = dbcs_buf;
        UBYTE *buf_ptr = buf;
        struct Bacterium *b = ywd->UVBact;
        LONG j,k,e,rc,found,do_reset = 0;
        ULONG i;
        struct OBNode *robo;
        LONG this_sec,this_min,this_hour,all_sec,all_min,all_hour;

        new_font(str,FONTID_TRACY);
        pos_brel(str,8,16);

        #ifndef YPA_CUTTERMODE

        switch( ywd->DebugInfo ) {

            case 1:

                /*** Vehicle-Statistics sammeln ***/
                yw_GetVehicleStats(ywd);

                /*** Versions-Nummer und Zeit ***/
                if (ywd->Version) {
                    str = yw_PutDbgMsg(ywd,str,&buf_ptr,"build id: %s",ywd->Version);
                    new_line(str);
                };
                this_sec  = (ywd->TimeStamp>>10);
                this_min  = this_sec/60;
                this_hour = this_min/60;
                if (!ywd->playing_network) {
                    all_sec  = ((ywd->GlobalStats[1].Time + ywd->TimeStamp)>>10);
                    all_min  = all_sec/60;
                    all_hour = all_min/60;
                } else {
                    all_sec = all_min = all_hour = 0;
                };
                str = yw_PutDbgMsg(ywd,str,&buf_ptr,"time: (this: %02d:%02d:%02d) (all: %02d:%02d:%02d)",this_hour,this_min%60,this_sec%60,all_hour,all_min%60,all_sec%60);
                new_line(str);

                /*** Profiling-Info ***/
                str = yw_PutDbgMsg(ywd,str,&buf_ptr,"prof all: %d",ywd->Profile[PROF_FRAMETIME]); new_line(str);
                str = yw_PutDbgMsg(ywd,str,&buf_ptr,"prof fprint: %d",ywd->Profile[PROF_SETFOOTPRINT]); new_line(str);
                str = yw_PutDbgMsg(ywd,str,&buf_ptr,"prof gui: %d",ywd->Profile[PROF_GUILAYOUT]); new_line(str);
                str = yw_PutDbgMsg(ywd,str,&buf_ptr,"prof ai: %d",ywd->Profile[PROF_TRIGGERLOGIC]); new_line(str);
                str = yw_PutDbgMsg(ywd,str,&buf_ptr,"prof rend: %d",ywd->Profile[PROF_RENDERING]); new_line(str);
                str = yw_PutDbgMsg(ywd,str,&buf_ptr,"prof net: %d",ywd->Profile[PROF_NETWORK]); new_line(str);

                /*** etwas Sektor-Info und User-Info ***/
                str = yw_PutDbgMsg(ywd,str,&buf_ptr,"sec type: %d",ywd->UVBact->Sector->Type); new_line(str);
                str = yw_PutDbgMsg(ywd,str,&buf_ptr,"sec wtype: %d",ywd->UVBact->Sector->WType); new_line(str);
                str = yw_PutDbgMsg(ywd,str,&buf_ptr,"num beam: %d",ywd->Level->MaxNumBuddies); new_line(str);

                /*** Vehicle-Anzahl-Nummern ***/
                str = yw_PutDbgMsg(ywd,str,&buf_ptr,"num sqd: %d,%d",ywd->AllSquads,ywd->MaxSquads);
                new_line(str);
                str = yw_PutDbgMsg(ywd,str,&buf_ptr,"num vhcl: %d,%d",ywd->AllVehicles,ywd->MaxVehicles);
                new_line(str);
                str = yw_PutDbgMsg(ywd,str,&buf_ptr,"num flk: %d,%d",ywd->AllFlaks,ywd->MaxFlaks);
                new_line(str);
                str = yw_PutDbgMsg(ywd,str,&buf_ptr,"num robo: %d,%d",ywd->AllRobos,ywd->MaxRobos);
                new_line(str);
                str = yw_PutDbgMsg(ywd,str,&buf_ptr,"num wpn: %d,%d",ywd->AllWeapons,ywd->MaxWeapons);
                new_line(str);
                str = yw_PutDbgMsg(ywd,str,&buf_ptr,"reload const: %d",ywd->URBact->RoboReloadConst);
                new_line(str);
            break;

        case 2:

            /*** Netstatus ***/
            if( ywd->gsr && ywd->playing_network ) {

                new_line( str ); // beisst sich sonst mit nettroublerequ.
                new_line( str );

                for( rc = 0; rc < MAXNUM_OWNERS; rc++ ) {

                    if( NWS_NOTHERE != ywd->gsr->player[ rc ].status ) {

                        char *n, *m;
                        switch( rc ) {
                            case 1: n = "Resistance"; break;
                            case 3: n = "Mykonier  ";   break;
                            case 4: n = "Taerkasten"; break;
                            case 6: n = "Ghorkov   ";    break;
                            default:n = "Hae?!     ";      break;
                            }

                        switch( ywd->gsr->player[ rc ].status ) {
                            case NWS_INGAME:   m = "OK";            break;
                            case NWS_REMOVED:  m = "Removed";       break;
                            case NWS_LEFTGAME: m = "left the game"; break;
                            case NWS_TROUBLE:  m = "makes trouble"; break;
                            default:           m = "???";           break;
                            }

                        str = yw_PutDbgMsg(ywd,str,&buf_ptr,"%s status: %s latency: %d", n, m, ywd->gsr->player[ rc ].latency); new_line( str );
                        }
                    }

                new_line( str );
                
                str = yw_PutDbgMsg(ywd,str,&buf_ptr, "net send: %d bytes/sec", ywd->gsr->transfer_sbps); new_line(str);
                str = yw_PutDbgMsg(ywd,str,&buf_ptr, "net rcv: %d bytes/sec", ywd->gsr->transfer_rbps); new_line(str);
                str = yw_PutDbgMsg(ywd,str,&buf_ptr, "packet: %d bytes", ywd->gsr->transfer_pckt); new_line(str);
                if( ywd->infooverkill )
                    str = yw_PutDbgMsg(ywd,str,&buf_ptr, "WARNING: INFO OVERKILL"); new_line(str);
                
                if( ywd->nwo ) {
                
                    struct diagnosis_msg dm;
                    
                    _methoda( ywd->nwo, NWM_DIAGNOSIS, &dm );
                    
                    new_line( str );
                    str = yw_PutDbgMsg(ywd,str,&buf_ptr, "thread send list now: %d", dm.num_send); new_line(str);
                    str = yw_PutDbgMsg(ywd,str,&buf_ptr, "thread recv list now: %d", dm.num_recv); new_line(str);
                    str = yw_PutDbgMsg(ywd,str,&buf_ptr, "thread send list max: %d", dm.max_send); new_line(str);
                    str = yw_PutDbgMsg(ywd,str,&buf_ptr, "thread recv list max: %d", dm.max_recv); new_line(str);
                    str = yw_PutDbgMsg(ywd,str,&buf_ptr, "send call now: %d", dm.tme_send); new_line(str);
                    str = yw_PutDbgMsg(ywd,str,&buf_ptr, "send call max: %d", dm.max_tme_send); new_line(str);
                    str = yw_PutDbgMsg(ywd,str,&buf_ptr, "send bugs: %d", dm.num_bug_send); new_line(str);
                    }
                }
            else {
                str = yw_PutDbgMsg(ywd,str,&buf_ptr,"not a network game");
                new_line( str );
                }

            break;
            
        case 3:
            {
                /*** Input-Info ***/
                UBYTE btn_buf[128];
                for (i=0; i<17; i++) {
                    str = yw_PutDbgMsg(ywd,str,&buf_ptr,"slider[%d] = %f",i,ip->Slider[i]);
                    new_line(str);
                };  
                for (i=0; i<32; i++) {
                    if (ip->Buttons & (1<<i))  btn_buf[i] = 'O';
                    else                       btn_buf[i] = '_';
                };
                btn_buf[i] = 0;
                str = yw_PutDbgMsg(ywd,str,&buf_ptr,btn_buf);
            };
            break;

        default:

            /*** Dann eben Debug-1-ten Robo anzeigen ***/
            rc    = 0;
            found = 0;
            for( robo = (struct OBNode *)ywd->CmdList.mlh_Head;
                 (robo->nd.mln_Succ) && (found == 0);
                 robo = (struct OBNode *) robo->nd.mln_Succ ) {

                if( (BCLID_YPAROBO == robo->bact->BactClassID) &&
                    (1             != robo->bact->Owner) ) {

                    struct yparobo_data *yrd;
                    char   *b, *v;

                    rc++;

                    if( rc < (ywd->DebugInfo-2) )
                        continue;

                    found = 1;

                    /*** Energy ***/
                    yrd  = INST_DATA( ((struct nucleusdata *)(robo->o))->o_Class, robo->o);
                    str = yw_PutDbgMsg(ywd,str, &buf_ptr, "robo owner %d with energy %d / %d / %d / %d",
                                        robo->bact->Owner, robo->bact->Energy,
                                        yrd->BuildSpare, yrd->VehicleSpare,
                                        robo->bact->Maximum);
                    new_line( str );

                    /*** aktuelles ***/
                    switch( yrd->BuildSlot_Kind ) {
                        case 0:                 b = "nothing";   break;
                        case ROBO_BUILDPOWER:   b = "powerstation"; break;
                        case ROBO_BUILDRADAR:   b = "radar"; break;
                        case ROBO_BUILDSAFETY:  b = "flak"; break;
                        case ROBO_CHANGEPLACE:  b = "location"; break;
                        default:                b = "powerstation"; break;
                        }

                    switch( yrd->VehicleSlot_Kind ) {
                        case 0:                 v = "nothing";   break;
                        case ROBO_BUILDDEFENSE: v = "defense"; break;
                        case ROBO_BUILDCONQUER: v = "conquer"; break;
                        case ROBO_BUILDRECON:   v = "recon"; break;
                        case ROBO_BUILDROBO:    v = "robo"; break;
                        default:                v = "powerstation"; break;
                        }

                    str = yw_PutDbgMsg(ywd,str,&buf_ptr,"    do build job   >%s<   and vhcl job   >%s<", b, v);
                    new_line( str );

                    /*** Verzoegerung ***/
                    str = yw_PutDbgMsg(ywd,str,&buf_ptr,"    wait power %d, radar %d, flak %d, location %d",
                                    yrd->chk_Power_Delay/1000, yrd->chk_Radar_Delay/1000,
                                    yrd->chk_Safety_Delay/1000,  yrd->chk_Place_Delay/1000);
                    new_line( str );

                    str = yw_PutDbgMsg(ywd,str,&buf_ptr,"    wait conquer %d, defense %d, recon %d, robo %d",
                                    yrd->chk_Terr_Delay/1000, yrd->chk_Enemy_Delay/1000,
                                    yrd->chk_Recon_Delay/1000,  yrd->chk_Robo_Delay/1000);
                    new_line( str );

                    /*** Was gefunden? ***/
                    str = yw_PutDbgMsg(ywd,str,&buf_ptr, "    values  ");
                    new_line( str );
                    if( yrd->chk_Power_Delay <= 0 )
                        str = yw_PutDbgMsg(ywd,str, &buf_ptr, "power %d, ", yrd->chk_Power_Value );
                    else
                        str = yw_PutDbgMsg(ywd,str,&buf_ptr, "power -1, ");
                    new_line( str );
                    if( yrd->chk_Radar_Delay <= 0 )
                        str = yw_PutDbgMsg(ywd,str,&buf_ptr, "radar %d, ", yrd->chk_Radar_Value );
                    else
                        str = yw_PutDbgMsg(ywd,str,&buf_ptr, "radar -1, ");
                    new_line( str );
                    if( yrd->chk_Safety_Delay <= 0 )
                        str = yw_PutDbgMsg(ywd,str, &buf_ptr, "flak %d, ", yrd->chk_Safety_Value );
                    else
                        str = yw_PutDbgMsg(ywd,str,&buf_ptr, "flak -1, ");
                    new_line( str );
                    if( yrd->chk_Place_Delay <= 0 )
                        str = yw_PutDbgMsg(ywd,str,&buf_ptr, "power %d, ", yrd->chk_Place_Value );
                    else
                        str = yw_PutDbgMsg(ywd,str,&buf_ptr, "power -1, ");
                    new_line( str );
                    //str += sprintf( str, "          ");
                    if( yrd->chk_Enemy_Delay <= 0 )
                        str = yw_PutDbgMsg(ywd,str,&buf_ptr, "defense %d, ", yrd->chk_Enemy_Value );
                    else
                        str = yw_PutDbgMsg(ywd,str,&buf_ptr, "defense -1, ");
                    new_line( str );
                    if( yrd->chk_Terr_Delay <= 0 )
                        str = yw_PutDbgMsg(ywd,str,&buf_ptr, "conquer %d, ", yrd->chk_Terr_Value );
                    else
                        str = yw_PutDbgMsg(ywd,str,&buf_ptr, "conquer -1, ");
                    new_line( str );
                    if( yrd->chk_Recon_Delay <= 0 )
                        str = yw_PutDbgMsg(ywd,str,&buf_ptr, "recon %d, ", yrd->chk_Recon_Value );
                    else
                        str = yw_PutDbgMsg(ywd,str,&buf_ptr, "recon -1, ");
                    new_line( str );
                    if( yrd->chk_Robo_Delay <= 0 )
                        str = yw_PutDbgMsg(ywd,str,&buf_ptr, "robo %d, ", yrd->chk_Robo_Value );
                    else
                        str = yw_PutDbgMsg(ywd,str,&buf_ptr, "robo -1, ");
                    new_line( str );
                    if( yrd->RoboState & ROBO_DOCKINUSE )
                        str = yw_PutDbgMsg(ywd,str,&buf_ptr, "dock energy %d time %d",
                              yrd->dock_energy, yrd->dock_time);
                }
            }

            if( !found ) do_reset = 1;
            break;
        }

        #endif
        new_line(str);
        str = yw_PutDbgMsg(ywd,str,&buf_ptr,"fps: %d",ywd->FramesProSec); new_line(str);
        str = yw_PutDbgMsg(ywd,str,&buf_ptr,"polys: %d,%d",ywd->PolysAll,ywd->PolysDrawn); new_line(str);
        #ifdef YPA_CUTTERMODE
            str = yw_PutDbgMsg(ywd,str,&buf_ptr,"frame: %d/%d",ywd->in_seq->frame.id,ywd->in_seq->num_frames); new_line(str);
        #endif
        eos(str);

        _methoda(ywd->GfxObject,RASTM_Begin2D,NULL);
        rt.string = dbcs_buf;
        rt.clips  = NULL;
        _methoda(ywd->GfxObject,RASTM_Text,&rt);
        _methoda(ywd->GfxObject,RASTM_End2D,NULL);

        if (do_reset == 0) {
            if(ip->NormKey == KEYCODE_F9) ywd->DebugInfo++;
        }else ywd->DebugInfo = 0;

    }else{
        if (ip->NormKey == KEYCODE_F9) ywd->DebugInfo++;
    };
}

/*-----------------------------------------------------------------*/
void yw_EnemySectorCheck(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Wenn User-Fahrzeug von einem eigenen oder
**      neutralen auf einen feindlichen (gefährlichen) Sektor
**      fährt, wird eine kurze Warnung ausgegeben.
**
**  CHANGED
**      03-Oct-96   floh    created
**      26-Sep-97   floh    Bugfix: war auf Owner 1 hardgecodet
**      29-Sep-97   floh    wird jetzt ausgebremst
*/
{
    ULONG newown = ywd->UVBact->Sector->Owner;
    ULONG oldown = ywd->UVSecOwnerBU;
    if (((newown!=ywd->URBact->Owner) && (newown!=0)) &&
        ((oldown==ywd->URBact->Owner) || (oldown==0)))
    {
        if ((ywd->TimeStamp - ywd->EnemySectorTimeStamp) > 10000) {
            struct logmsg_msg lmm;
            lmm.bact = ywd->UVBact;
            lmm.pri  = 24;
            lmm.msg  = ypa_GetStr(ywd->LocHandle,STR_LMSG_ENEMYSECTOR,"ENEMY SECTOR ENTERED");
            lmm.code = LOGMSG_ENEMYSECTOR;
            _methoda(ywd->world, YWM_LOGMSG, &lmm);
        };
        ywd->EnemySectorTimeStamp = ywd->TimeStamp;
    };
}

/*-----------------------------------------------------------------*/
void yw_TimerCallbackStub(void *arg)
/*
**  FUNCTION
**      Callback-Stub für die EditBox-Input-Routine, ruft
**      einfach ypa_DoFrame() aus ypa.c auf. Vorher muß
**      ywd->DontRender angeschaltet werden!
**
**  CHANGED
**      14-Jan-98   floh    created
*/
{
    ypa_DoFrame();
}

/*-----------------------------------------------------------------*/
_dispatcher(void, yw_BSM_TRIGGER, struct trigger_msg *msg)
/*
**  FUNCTION
**      Überschreibt die BSM_TRIGGER-Methode der base.class.
**      Wird einmal pro Frame (== Spiel-Zyklus) auf das Welt-
**      Object angewendet. Grob gesagt, wird zuerst die gesamte
**      Simulation erledigt, dann das Rendern des Frames.
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      11-May-95   floh    created
**      11-Jun-95   floh    verteilt jetzt ein YBM_TR_LOGIC an alle
**                          Commander
**      28-Jun-95   floh    Zell-Automaten-Pässe wieder entfernt
**      21-Jul-95   floh    neu: Energie-Handling.
**      24-Jul-95   floh    Energie-Handling leicht modifiziert
**      29-Jul-95   floh    Energie-Handling wird jetzt nur ausgeführt,
**                          wenn YWA_DoEnergyCycle auf TRUE
**      13-Aug-95   floh    GUI-Input-Handling via yw_HandleGUIInput()
**      14-Aug-95   floh    yw_LayoutGUI(), außerdem wird
**                          Framerate berechnet und nach ywd->FramesProSec
**                          geschrieben.
**      20-Sep-95   floh    Zuallererst wird jetzt YBM_TR_LOGIC
**                          auf alle Commanders angewendet, um irgendwelchen
**                          nicht definierten Zuständen vorzubeugen.
**      06-Nov-95   floh    neu: yw_RenderMap()
**      29-Dec-95   floh    Mouse-Selektions-Zeugs...
**      03-Jan-96   floh    TLMsg jetzt statischer Bestandteil der
**                          Welt-LID, weil das GUI-Selectingdie User Action
**                          reinschreiben muß.
**      29-Jan-96   floh    ruft jetzt regelmäßig yw_DoBuildJobs() auf
**      11-Mar-96   floh    AF's Footprint-Schleife vom 27-Feb übernommen
**      21-Mar-96   floh    Footprints werden jetzt zuerst korrekt
**                          initialisiert, außerdem wurde die SETFOOTPRINT-
**                          Schleife vor allem anderen platziert
**      03-Apr-96   floh    + Digitizing-Support
**      26-Apr-96   floh    + _StartAudioFrame(), _EndAudioFrame()
**      10-May-96   floh    + ywd->TimeStamp Handling
**      17-May-96   floh    + schreibt jetzt als allererste Amtshandlung
**                            den Pointer auf die TriggerLogicMsg in
**                            die LID, damit er nicht bei jeder möglichen
**                            und unmöglichen Gelegenheit mitgeschleift
**                            werden muß!
**      19-May-96   floh    + oops, die FrameTime-Berechnung war broken.
**      24-May-96   floh    + im Design-Modus wird die Energy des
**                            User-Robos in jedem Frame auf MaxEnergy
**                            gesetzt.
**      28-May-96   floh    + _RefreshSoundCarrier() auf die GUI-Noises
**      15-Jun-96   floh    + yw_InputControl()
**      24-Jun-96   floh    + yw_BeamGateCheck()
**      07-Jul-96   floh    + yw_RCNewFrame(), yw_RCEndFrame() (Scene-Recorder)
**      21-Jul-96   floh    + yw_GetRealViewerPos()
**      19-Aug-96   floh    + Rendering neu organisiert, direkte
**                            Benutzung des Gfx-Objects
**      22-Aug-96   floh    + "richtiger" Mausblanker.
**      08-Sep-96   floh    + yw_PutDebugInfo()
**      21-Sep-96   floh    + Designmodus: unbegrenzte Vehikel-Energie
**      28-Sep-96   floh    + Bugfix: Audio-Ausgabe wurde nicht mit
**                            der "resultierenden" Viewer-Pos und -Matrix
**                            abgehandelt.
**      01-Oct-96   floh    + beachtet jetzt LEVELSTAT_PAUSED (in diesem
**                            Fall wird die Frame-Time einfach auf
**                            1 gesetzt, und das Spiel künstlich
**                            auf 10 Frames pro Sekunde verzögert.
**      05-Nov-96   floh    + yw_energy.c/yw_ComputeRatios()
**      13-Dec-96   floh    + Trigger-Message für die Bacts bekommt
**                            ywd->TimeStamp als GlobalTime
**                          + ywd->FrameCount++
**                          + FadeIn nachdem der 1.Frame gezeichnet
**                            wurde (und ein "Dummy-GetInput" um den
**                            FrameTimer zurückzusetzen).
**      05-Apr-97   floh    + erweitert um AF's Network-Sachen
**      22-May-97   floh    + yw_FFTrigger() (ForceFeedback-Support)
**      13-Aug-97   floh    + ywd->FrameTime wird ausgefüllt
**      25-Aug-97   floh    + macht ein YPAHIST_NEWFRAME für die
**                            Debriefing-History
**      30-Sep-97   floh    + FadeIn() Effekt rausgenommen.
**      27-Oct-97   floh    + yw_CheckPalFXBug() eingefügt
**      04-Dec-97   floh    + yw_RemapVehicles() und yw_RemapBuildings() ersetzt
**                            durch yw_RemapThings()
**      14-Jan-98   floh    + beachtet jetzt ywd->DontRender, damit die
**                            Trigger-Methode auch vom TimerService des
**                            Textinput-Feldes für die japanische Version
**                            aufgerufen werden kann
**      07-Feb-98   floh    + Wenn User auf Sektor 0,0 ist, wird nix
**                            gerendert.
**      09-Feb-98   floh    + Frametime-Patch raus, wenn Abort-Req auf.
**      11-Feb-98   floh    + yw_TriggerSuperItems()
**      18-Apr-98   floh    + yw_TriggerEventCatcher()
*/
{
    struct ypaworld_data *ywd = INST_DATA(cl,o);
    struct MinList *ls;
    struct MinNode *nd;
    struct MinNode *next;
    Object *cmd;
    struct flt_triple vwr_vec;
    struct flt_m3x3 *rm;
    struct flt_m3x3 nm;
    struct tform *vwr;
    struct ypa_HistNewFrame hnf;

    /*** Profile Data ***/
    ULONG prof_frametime;
    ULONG prof_setfootprint;
    ULONG prof_guilayout;
    ULONG prof_triggerlogic;
    ULONG prof_rendering;
    ULONG prof_network1;
    ULONG prof_network2;

    /***-----------------------***/
    /*** Pausenmodus abhandeln ***/
    /***-----------------------***/
    if (yw_HandleGamePaused(ywd,msg)) return;

    #ifdef YPA_DESIGNMODE
        if (ywd->URBact) {
            ywd->URBact->Maximum = 10000000;
            ywd->URBact->Energy  = ywd->URBact->Maximum;
            ywd->URBact->RoboReloadConst = 20000000;
        };
        if (ywd->UVBact) {
            ywd->UVBact->Maximum = 10000000;
            ywd->UVBact->Energy  = ywd->UVBact->Maximum;
        };
    #endif

    /*** PROFILE ***/
    prof_frametime = yw_StartProfile();

    /*** GfxObject Pointer besorgen ***/
    _OVE_GetAttrs(OVET_Object, &(ywd->GfxObject), TAG_DONE);

    /*** Pointer auf Msg nach LID ***/
    ywd->TriggerMsg = msg;

    /*** Chatmodus erstmal auf normal ***/
    ywd->chat_system = FALSE;

    /*** Frametime auf 1/25 sec patchen, wenn Sequenz-Digitizing on ***/
    if (ywd->seq_digger) {
        msg->global_time -= msg->frame_time;
        msg->input->FrameTime = 40;
        msg->frame_time       = 40;
        msg->global_time += msg->frame_time;
    };

    /*** Verwaltungs-Kram ***/
    yw_ComputeRatios(ywd);
    yw_CheckIfUserSitsInRoboFlak(ywd);
    yw_InputControl(ywd, msg->input);

    /*** Mouse-Blanker ***/
    if (ywd->MouseBlanked) {
        struct ClickInfo *ci = &(msg->input->ClickInfo);
        if ((ywd->BMouseX != ci->act.scrx) ||
            (ywd->BMouseY != ci->act.scry))
        {
            ywd->MouseBlanked = FALSE;
        };
    }else{
        /*** Waypoint-Button schaltet Blanker NICHT ein! ***/
        if ((msg->input->NormKey != 0) &&
            (msg->input->NormKey != WINP_CODE_LMB) &&
            (msg->input->NormKey != WINP_CODE_MMB) &&
            (msg->input->NormKey != WINP_CODE_RMB) &&
            (!(msg->input->Buttons & (1<<4))))
        {
            struct ClickInfo *ci = &(msg->input->ClickInfo);
            ywd->MouseBlanked = TRUE;
            ywd->BMouseX = ci->act.scrx;
            ywd->BMouseY = ci->act.scry;
        };
    };

    /*** allgemeine Frame-Initialisierungs-Sachen ***/
    ywd->TimeStamp += msg->frame_time;
    ywd->FrameTime  = msg->frame_time;
    ywd->FrameCount++;
    ywd->TLMsg.user_action = YW_ACTION_NONE;
    ywd->TLMsg.global_time = ywd->TimeStamp;
    ywd->TLMsg.frame_time  = msg->frame_time;
    ywd->TLMsg.input       = msg->input;
    ywd->TLMsg.slave_count = 0;
    ywd->FramesProSec = 1024 / msg->frame_time;
    ywd->Profile[PROF_FRAME] = ywd->FramesProSec;

    /*** Debriefing History ***/
    hnf.cmd        = YPAHIST_NEWFRAME;
    hnf.time_stamp = ywd->TimeStamp;
    _methoda(ywd->world, YWM_NOTIFYHISTORYEVENT, &hnf);

    prof_network1 = yw_StartProfile();
    if( ywd->playing_network ) {

        yw_NetMessageLoop( ywd );

        /*** Soll das Spiel verlassen werden? ***/
        if( LEVELSTAT_ABORTED == ywd->Level->Status )
            return;
        }
    ywd->Profile[PROF_NETWORK] = yw_EndProfile(prof_network1);

    /***---------------------------------***/
    /***--- Footprints initialisieren ---***/
    /***---------------------------------***/
    prof_setfootprint = yw_StartProfile();
    yw_ClearFootPrints(ywd);
    ls = &(ywd->CmdList);
    nd = ls->mlh_Head;
    while(nd->mln_Succ) {

        /*** Abdruck... ***/
        cmd = ((struct OBNode *)nd)->o;
        _methoda( cmd, YBM_SETFOOTPRINT, NULL);

        /*** NeXT one ***/
        nd = nd->mln_Succ;
    };
    ywd->Profile[PROF_SETFOOTPRINT] = yw_EndProfile(prof_setfootprint);

    /***--------------------------***/
    /***--- Bildschirm löschen ---***/
    /***--------------------------***/
    if (!ywd->DontRender) {
        _methoda(ywd->GfxObject,DISPM_Begin,0);
        _set(ywd->GfxObject, RASTA_BGPen, 0);
        _methoda(ywd->GfxObject, RASTM_Clear, NULL);
    };

    /***------------------------------***/
    /***--- interne Arrays updaten ---***/
    /***------------------------------***/
    yw_RemapWeapons(ywd);
    yw_RemapVehicles(ywd);
    yw_RemapBuildings(ywd);
    yw_RemapCommanders(ywd);
    #ifdef YPA_DESIGNMODE
        yw_RemapSectors(ywd);
    #endif

    /***-----------------------------------***/
    /***--- Audio-Frame-Initialisierung ---***/
    /***-----------------------------------***/
    vwr_vec.x = ywd->UVBact->dof.x * ywd->UVBact->dof.v;
    vwr_vec.y = ywd->UVBact->dof.y * ywd->UVBact->dof.v;
    vwr_vec.z = ywd->UVBact->dof.z * ywd->UVBact->dof.v;
    _StartAudioFrame(msg->frame_time,
                     &(ywd->ViewerPos),
                     &(vwr_vec),
                     &(ywd->ViewerDir));
    if (ywd->FrameCount == 1) {
        struct logmsg_msg lm;
        lm.bact = ywd->URBact;
        lm.pri  = 60;
        lm.msg  = NULL;
        lm.code = LOGMSG_HOST_ONLINE;
        _methoda(ywd->world,YWM_LOGMSG,&lm);
    };

    /***-----------------------------------------***/
    /***--- BeamGates und BuildJobs abhandeln ---***/
    /***-----------------------------------------***/
    yw_BeamGateCheck(ywd);
    yw_TriggerSuperItems(ywd);
    yw_DoBuildJobs(o, ywd, msg->frame_time);

    /***-------------------------------------***/
    /***--- GUI Layout und Input Handling ---***/
    /***-------------------------------------***/
    prof_guilayout = yw_StartProfile();
    yw_LayoutGUI(o, ywd);
    yw_HandleGUIInput(ywd, msg->input);
    yw_GetRealViewerPos(ywd);
    yw_RenderMap(ywd);
    yw_BuildTrLogicMsg(o,ywd,msg->input);
    ywd->Profile[PROF_GUILAYOUT] = yw_EndProfile(prof_guilayout);

    /*** Energy-Handling ***/
    if (ywd->DoEnergyCycle) yw_Energize(ywd, msg->frame_time);

    /*** falls Recording, neuen Frame starten ***/
    if (ywd->out_seq->active) yw_RCNewFrame(ywd,msg->frame_time);

    /*** Visier ausschalten (kann per YWM_VIZOR eingeschaltet werden) ***/
    ywd->visor.mg_type  = VISORTYPE_NONE;
    ywd->visor.gun_type = VISORTYPE_NONE;

    /*** User-Vehicle-Sector-Owner aufheben ***/
    ywd->UVSecOwnerBU = ywd->UVBact->Sector->Owner;

    /***--------------------------------------***/
    /***--- YBA_TR_LOGIC an alle Commander ---***/
    /***--------------------------------------***/
    prof_triggerlogic = yw_StartProfile();
    nd = ls->mlh_Head;
    while(nd->mln_Succ) {

        /*** vorher Nachfolger nehmen ***/
        next = nd->mln_Succ;

        /*** Triggern ***/
        cmd = ((struct OBNode *)nd)->o;

        if( ywd->playing_network ) {

            if( (cmd == ywd->UserRobo) ||
                (((struct OBNode *)nd)->bact->BactClassID != BCLID_YPAROBO ) )
                _methoda(cmd, YBM_TR_LOGIC, &(ywd->TLMsg));
            else
                _methoda(cmd, YBM_TR_NETLOGIC, &(ywd->TLMsg));
            }
        else
            _methoda(cmd, YBM_TR_LOGIC, &(ywd->TLMsg));

        ywd->TLMsg.slave_count++;

        /*** Nachfolger ***/
        nd = next;
    };
    ywd->Profile[PROF_TRIGGERLOGIC] = yw_EndProfile(prof_triggerlogic);

    /***---------------------------------------***/
    /***--- ViewerPos auf aktuellsten Stand!   ***/
    /***---------------------------------------***/
    yw_GetRealViewerPos(ywd);

    /***---------------------------------------***/
    /***--- Netzwerk-Message-Puffer flushen ---***/
    /***---------------------------------------***/
    prof_network2 = yw_StartProfile();
    if( ywd->playing_network ) {

        if( msg->frame_time == 1 )
            ywd->flush_time -= 20;
        else
            ywd->flush_time -= msg->frame_time;

        if( ywd->flush_time <= 0 ) {

            /*** nach dem Triggern Buffer für MSG_ALL-Messages leeren ***/
            struct flushbuffer_msg fb;
            LONG   bytes;

            fb.sender_kind   = MSG_PLAYER;
            fb.sender_id     = ywd->gsr->NPlayerName;
            fb.receiver_kind = MSG_ALL;
            fb.receiver_id   = NULL;
            fb.guaranteed    = FALSE;   // weil ingame-zeug, keine Initdaten

            bytes = _methoda( ywd->nwo, NWM_FLUSHBUFFER, &fb);

            /*** statistik ***/
            ywd->gsr->transfer_sendcount += bytes;
            if( ywd->gsr->transfer_pckt_min == 0 )
                ywd->gsr->transfer_pckt_min = bytes;
            else {
                if( ywd->gsr->transfer_pckt_min > bytes )
                    ywd->gsr->transfer_pckt_min = bytes;
                }
            if( ywd->gsr->transfer_pckt_max < bytes )
                ywd->gsr->transfer_pckt_max = bytes;
            if( bytes )
                ywd->gsr->transfer_pckt_count++;

            if( REDUCE_DATA_RATE )
                ywd->flush_time  = FLUSH_TIME_TROUBLE;
            else
                ywd->flush_time  = ywd->gsr->flush_time_normal;
            }
        }
    ywd->Profile[PROF_NETWORK] += yw_EndProfile(prof_network2);

    /*** GUI-Noises und VoiceOvers ***/
    if (ywd->UVBact && (ywd->gsr)) {
        if (ywd->gsr) {
            ywd->gsr->ShellSound1.pos = ywd->UVBact->pos;
            ywd->gsr->ShellSound2.pos = ywd->UVBact->pos;
            _RefreshSoundCarrier(&(ywd->gsr->ShellSound1));
            _RefreshSoundCarrier(&(ywd->gsr->ShellSound2));
        };
    };
    yw_TriggerEventCatcher(ywd);
    yw_TriggerVoiceOver(ywd);

    if (ywd->playing_network) {
        if (ywd->UVBact) ywd->gsr->ChatSound.pos = ywd->UVBact->pos;
        _RefreshSoundCarrier(&(ywd->gsr->ChatSound));
    };

    /*** Audio-Frame beenden, Shake-FX beachten ***/
    rm  = _EndAudioFrame();
    vwr = _GetViewer();
    nc_m_mul_m(rm,&(vwr->loc_m),&nm);
    vwr->loc_m = nm;

    /*** falls Recording, Frame beenden und nach File flushen ***/
    if (ywd->out_seq->active) yw_RCEndFrame(ywd);

    /*** YouEnteredAnEnemySector-Test ***/
    yw_EnemySectorCheck(ywd);

    /***-----------------------------------------------***/
    /***--- Frame rendern und Rendering abschließen ---***/
    /***-----------------------------------------------***/
    if (!ywd->DontRender) {
        prof_rendering = yw_StartProfile();
        if (!((ywd->UVBact->SectX==0) && (ywd->UVBact->SectY==0))) {
            yw_RenderFrame(o, ywd, msg, TRUE);
            yw_PutDebugInfo(ywd, msg->input);

            if( ywd->playing_network )
                yw_DrawNetworkStatusInfo( ywd );
        };
        _methoda(ywd->GfxObject, DISPM_End, NULL);
        ywd->Profile[PROF_RENDERING] = yw_EndProfile(prof_rendering);
    };

    /*** Force-Feedback-Handling ***/
    yw_FFTrigger(ywd);

    /*** Snapshot, Sequenz-Digitalisierung ***/
    yw_DoDigiStuff(ywd, msg->input);

    #ifdef YPA_DESIGNMODE
    /*** Maps auf Keypress sichern ***/
    if (KEYCODE_NUM_PLUS == msg->input->NormKey) {
        if (!yw_DesignSaveMaps(ywd)) {
            _LogMsg("DESIGNER ERROR! Execution terminated.\n");
            ywd->Level->Status = LEVELSTAT_FINISHED;
        };
    };
    /*** Höhenprofil auf Keypress sichern ***/
    if (KEYCODE_NUM_5 == msg->input->NormKey) {
        if (!yw_DesignSaveProfile(ywd)) {
            _LogMsg("DESIGNER ERROR! Failed to save height profile map!\n");
            ywd->Level->Status = LEVELSTAT_FINISHED;
        };
    };
    #endif

    ywd->Profile[PROF_FRAMETIME] = yw_EndProfile(prof_frametime);
    ywd->Profile[PROF_POLYGONS]  = ywd->PolysDrawn;
    yw_TriggerProfiler(ywd);

    /*** Chat-System ***/
    if ((!ywd->DontRender) && (ywd->chat_system)) {
        struct windd_gettext txtm;

        char headline[ 300 ];

        /*** Rendering abschalten ***/
        ywd->DontRender = TRUE;

        sprintf( headline, "%s %s", ypa_GetStr( GlobalLocaleHandle,
                                                STR_NET_MESSAGETO,
                                                "MESSAGE TO"),
                                                ywd->chat_name );

        /*** Texteingabe... ***/
        txtm.title_text   = headline;
        txtm.ok_text      = ypa_GetStr( GlobalLocaleHandle, STR_OK, "OK");
        txtm.cancel_text  = ypa_GetStr( GlobalLocaleHandle, STR_CANCEL, "CANCEL");
        txtm.default_text = "";
        txtm.result       = NULL;
        txtm.timer_val    = 250;
        txtm.timer_func   = yw_TimerCallbackStub;
        txtm.timer_arg    = (void *) ywd;
        _methoda(ywd->GfxObject,WINDDM_GetText,&txtm);

        /*** Rendering wieder an ***/
        ywd->DontRender = FALSE;

        /*** Eine Message losschicken ***/
        if (txtm.result) {

            yw_SendChatMessage( ywd, txtm.result, ywd->chat_name,
                                ywd->chat_owner);
        };
    };

    /*** Online-Help anzeigen? ***/
    if (ywd->Url) {
        struct yw_onlinehelp_msg ohm;
        ohm.url  = ywd->Url;
        ywd->Url = NULL;
        _methoda(ywd->world,YWM_ONLINEHELP,&ohm);
    };
}


