/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_select.c,v $
**  $Revision: 38.29 $
**  $Date: 1998/01/06 16:26:49 $
**  $Locker: floh $
**  $Author: floh $
**
**  Selektion mit Mouse in 3D-Welt und 2D-Karte.
**
**  (C) Copyright 1995 by A.Weissflog
*/
#include <exec/types.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "input/ie.h"
#include "engine/engine.h"
#include "bitmap/ilbmclass.h"
#include "ypa/ypagui.h"
#include "ypa/guimap.h"
#include "ypa/guifinder.h"
#include "ypa/ypabactclass.h"
#include "ypa/yparoboclass.h"
#include "ypa/ypavehicles.h"
#include "ypa/ypakeys.h"
#include "ypa/ypaworldclass.h"
#include "ypa/ypatooltips.h"

#include "yw_protos.h"

#define GEN_DIST (200.0)    // Entfernung für New und Add

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_input_engine
_extern_use_audio_engine

extern struct YPAStatusReq SR;
extern struct YPAMapReq MR;
extern struct YPAFinder FR;

/*-----------------------------------------------------------------*/
BOOL yw_InitMouse(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Lädt alle Mousepointer-Images.
**
**  CHANGED
**      29-Dec-95   floh    created
**      04-Jan-95   floh    neue Mouse-Pointer
**      15-Jan-97   floh    Pointer-Cleanup
**      30-May-97   floh    wieder mehr Pointer definiert
*/
{
    ULONG i;
    UBYTE old_path[255];
    struct disp_pointer_msg dpm;
    strcpy(old_path, _GetAssign("rsrc"));
    _SetAssign("rsrc","data:mc2res");
    for (i=0; i<NUM_MOUSE_POINTERS; i++) {

        UBYTE *str;

        switch(i) {
            case YW_MOUSE_POINTER:  str="pointers/pointer.ilbm";  break;
            case YW_MOUSE_CANCEL:   str="pointers/cancel.ilbm";   break;
            case YW_MOUSE_SELECT:   str="pointers/select.ilbm";   break;
            case YW_MOUSE_ATTACK:   str="pointers/attack.ilbm";   break;
            case YW_MOUSE_GOTO:     str="pointers/goto.ilbm";     break;
            case YW_MOUSE_DISK:     str="pointers/disk.ilbm";     break;
            case YW_MOUSE_NEW:      str="pointers/new.ilbm";      break;
            case YW_MOUSE_ADD:      str="pointers/add.ilbm";      break;
            case YW_MOUSE_CONTROL:  str="pointers/control.ilbm";  break;
            case YW_MOUSE_BEAM:     str="pointers/beam.ilbm";     break;
            case YW_MOUSE_BUILD:    str="pointers/build.ilbm";    break;
            default:                str="pointers/pointer.ilbm";  break;
        };
        ywd->MousePointer[i] = _new("ilbm.class",
                                    RA_Name, str,
                                    BMA_Texture,   TRUE,
                                    BMA_TxtBlittable, TRUE,
                                    TAG_DONE);
        if (ywd->MousePointer[i]) {
            _get(ywd->MousePointer[i], BMA_Bitmap, &(ywd->MousePtrBmp[i]));
        } else {
            _LogMsg("yw_select.c/yw_InitMouseStuff()\n");
            _SetAssign("rsrc",old_path);
            return(FALSE);
        };
    };

    dpm.pointer = NULL;
    dpm.type    = 0;
    _methoda(ywd->GfxObject,DISPM_SetPointer,&dpm);
    dpm.pointer = ywd->MousePtrBmp[YW_MOUSE_POINTER];
    dpm.type    = DISP_PTRTYPE_NORMAL;
    _methoda(ywd->GfxObject,DISPM_SetPointer,&dpm);
    _SetAssign("rsrc",old_path);

    /*** Ende ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void yw_KillMouse(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Killt alle Pointer-Images.
**
**  CHANGED
**      29-Dec-95   floh    created
**      13-Mar-98   floh    + schaltet jetzt vorher Mauspointer ab
*/
{
    ULONG i;
    if (ywd->GfxObject) {
        struct disp_pointer_msg dpm;
        dpm.pointer = NULL;
        dpm.type    = DISP_PTRTYPE_NONE;
        _methoda(ywd->GfxObject,DISPM_SetPointer,&dpm);
    };
    for (i=0; i<NUM_MOUSE_POINTERS; i++) {
        if (ywd->MousePointer[i]) {
            _dispose(ywd->MousePointer[i]);
            ywd->MousePointer[i] = NULL;
            ywd->MousePtrBmp[i]  = NULL;
        };
    };
}

/*-----------------------------------------------------------------*/
void yw_PrepareMouseSelect(struct ypaworld_data *ywd, struct VFMInput *ip)
/*
**  FUNCTION
**      Setzt bzw. löscht die <ywd->FrameFlags>
**          YWFF_DoSectorCheck
**          YWFF_DoBactCheck
**          YWFF_MouseOverGUI
**          YWFF_MouseOverMap
**          YWFF_MouseOverFinder
**
**  INPUTS
**      ywd -> Pointer auf LID des Welt-Objects
**      ip  -> Pointer auf VFMInput-Struktur für diesen Frame
**
**  RESULTS
**      siehe FUNCTION
**
**  CHANGED
**      29-Dec-95   floh    created
**      20-Mar-96   floh    + YWFF_MouseOverFinder
**                          + Modifikator YWFF_GimmeLeader, falls nur
**                            der Anführer, bzw. das Geschwader als
**                            Ganzes interessiert
**      23-May-96   floh    + Designer-Stuff
**                          + Selektion im Radarmodus ist jetzt
**                            wieder angeschaltet.
**      24-May-96   floh    - Panic-Modus
**      22-Jul-96   floh    + Fight-Modus vollkommen umgekrempelt
**      14-Sep-96   floh    + Sonderfall Map im Radarmodus gekillt
**      19-Sep-96   floh    + YWFF_MouseOverFinder-Test etwas genauer
**      08-Dec-97   floh    + MouseOverFinder Auswahl etwas relaxter
*/
{
    struct ClickInfo *ci = &(ip->ClickInfo);

    /*** FrameFlags erstmal löschen ***/
    ywd->FrameFlags = 0;

    /*** wo ist die Maus? ***/
    if (ci->flags & CIF_VALID) {

        if (ci->box) {

            /*** Mouse ist über irgendeinem Requester ***/
            ywd->FrameFlags |= YWFF_MouseOverGUI;

            if (ci->box == &(MR.req.req_cbox)) {
                if (ci->btn == MAPBTN_MAPINTERIOR) {
                    /*** Mouse ist über Map! ***/
                    ywd->FrameFlags |= YWFF_MouseOverMap;
                };

            } else if (ci->box == &(FR.l.Req.req_cbox)) {
                /*** Mouse ist über Finder, aber auch über einem Item? ***/
                if (ci->btn >= LV_NUM_STD_BTN) {
                    ULONG entry = ci->btn - LV_NUM_STD_BTN + FR.l.FirstShown;
                    if (entry < FR.l.NumEntries) {
                        ywd->FrameFlags |= YWFF_MouseOverFinder;
                    };
                };
            };
        };

        /*** welche Mouse-Selektions-Checks vornehmen? ***/
        switch (SR.ActiveMode) {
            case STAT_MODEF_ORDER:
            case STAT_MODEF_NEW:
            case STAT_MODEF_ADD:
            case STAT_MODEF_BUILD:
            case STAT_MODEF_AUTOPILOT:
                ywd->FrameFlags |= (YWFF_DoSectorCheck|
                                    YWFF_DoBactCheck|
                                    YWFF_GimmeLeader);
                break;

            case STAT_MODEF_CONTROL:
                ywd->FrameFlags |= YWFF_DoBactCheck;
                break;

            #ifdef YPA_DESIGNMODE
            case STAT_MODEF_SETSECTOR:
            case STAT_MODEF_SETHEIGHT:
            case STAT_MODEF_SETOWNER:
                ywd->FrameFlags |= YWFF_DoSectorCheck;
                break;
            #endif

            #ifdef YPA_CUTTERMODE
            case STAT_MODEF_FREESTATIV:
                ywd->FrameFlags |= YWFF_DoSectorCheck;
                break;
            case STAT_MODEF_LINKSTATIV:
                ywd->FrameFlags |= (YWFF_DoSectorCheck|YWFF_DoBactCheck);
                break;
            case STAT_MODEF_ONBOARD1:
            case STAT_MODEF_ONBOARD2:
                ywd->FrameFlags |= YWFF_DoBactCheck;
                break;
            #endif
        };
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yw_DoMapSelect(struct ypaworld_data *ywd, struct ClickInfo *ci)
/*
**  FUNCTION
**      Darf nur aufgerufen werden, wenn YWFF_MouseOverMap
**      gesetzt ist! Übernimmt die potentielle Selektion
**      für genau diesen Fall.
**
**  CHANGED
**      29-Dec-95   floh    created + debugging
**      30-Dec-95   floh    tote und Genesis-Bacts werden ignoriert
**      03-Jan-95   floh    bei Sector-Select werden die X/Z-Koordinaten
**                          der Mouse in der Welt nach SelSecIPos()
**                          geschrieben, Y wird auf Sektor-Height gesetzt
**      13-Mar-96   floh    Anomalie: der Extra-State-Check beim
**                          Bakterien-Ignorieren war wrong.
**      14-Jun-96   floh    + größerer Selektions-Bereich für Bakterien
**                          + Bakterien-Selektion in unbekannten
**                            Sektoren nicht mehr möglich (Footprint-Check)
**      20-Jun-96   floh    + Flaks werden jetzt außer CONTROL und FIGHT
**                            ignoriert.
**      26-Jun-96   floh    + Fixes für "Map kleiner als Map-Window"
**      02-Aug-96   floh    + die IPos wird jetzt in [x,z] 
**                            verschliffen (+ 0.5), um die "No-Intersection-
**                            Anomalie" auszuschalten.
**      16-Sep-96   floh    + revised & updated
**      22-Sep-96   floh    + Selections am Weltrand sind nicht mehr
**                            erlaubt
**      14-Oct-96   floh    + ACTION_BEAM Fahrzeuge werden ignoriert
**      08-Dec-97   floh    + in Build und Beam werden Fahrzeuge
**                            komplett ignoriert
**      14-Jan-98   floh    + Flaks werden als possibly selected
**                            zugelassen, damit Doppelklick in
**                            Flaks funktioniert. Ignoriert werden
**                            sie dann aber in yw_ActionSelect()
**      06-mar-98   floh    + Selektions-Bereich mal auf 16 Pixel
**                            vergrößert
*/
{
    struct Cell *msec_ptr;

    /*** Sektor herausfinden, über dem die Mouse hängt ***/
    LONG mp_x = (MR.midx / MR.x_aspect)  - (MR.pw_x>>1) + ci->act.btnx;
    LONG mp_y = -(MR.midz / MR.y_aspect) - (MR.pw_y>>1) + ci->act.btny;

    /*** siehe yw_maprnd.c/yw_RenderMapVehicles()... ***/
    LONG secsize_shift = MR.zoom + 2;    // 3..6 (== 8..64 Pixel je Sektor)

    LONG msec_x = mp_x >> secsize_shift;
    LONG msec_y = mp_y >> secsize_shift;

    /*** außerhalb der Map ***/
    if ((msec_x<1)||(msec_y<1) ||
        (msec_x>=(ywd->MapSizeX-1))||
        (msec_y>=(ywd->MapSizeY-1)))
    {
        return;
    };

    msec_ptr = &(ywd->CellArea[msec_y * ywd->MapSizeX + msec_x]);

    /*** die Mouse ist über Sektor (msec_x,msec_y) ***/
    if (ywd->FrameFlags & YWFF_DoSectorCheck) {
        ywd->FrameFlags |= YWFF_MouseOverSector;
        ywd->SelSector   = msec_ptr;
        ywd->SelSecX     = msec_x;
        ywd->SelSecY     = msec_y;
        /*** Intersection-Point ***/
        ywd->SelSecIPos.x = mp_x * MR.x_aspect + 0.5;
        ywd->SelSecIPos.y = msec_ptr->Height;
        ywd->SelSecIPos.z = -(mp_y * MR.y_aspect + 0.75);
        /*** Sektor-Mittelpunkt ***/
        ywd->SelSecMPos.x = (FLOAT) (msec_x * SECTOR_SIZE + SECTOR_SIZE/2);
        ywd->SelSecMPos.y = msec_ptr->Height;
        ywd->SelSecMPos.z = (FLOAT) -(msec_y * SECTOR_SIZE + SECTOR_SIZE/2);
    };

    /*** evtl. noch den Bakterien-Check ***/
    if ((ywd->FrameFlags & YWFF_DoBactCheck) &&
        (!(SR.ActiveMode & (STAT_MODEF_BUILD|STAT_MODEF_AUTOPILOT))) &&
        (msec_ptr->FootPrint & (1<<ywd->URBact->Owner)))
    {
        /*** sie yw_maprnd.c/yw_RenderMapVehicles() ***/
        ULONG pass = 0;

        /*** 2 Pässe, zuerst User, dann Feindvehicle ***/
        for (pass=0; pass<2; pass++) {

            struct MinList *ls;
            struct MinNode *nd;

            ls = (struct MinList *) &(msec_ptr->BactList);
            for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {

                struct Bacterium *b = (struct Bacterium *) nd;
                LONG bpx, bpy;

                /*** Feindvehikel im ersten Pass ignorieren ***/
                if ((pass==1)&&(b->Owner==ywd->URBact->Owner))      continue;
                else if ((pass==0)&&(b->Owner!=ywd->URBact->Owner)) continue;

                /*** Ignoranz-Bedingungen ***/
                #ifndef YPA_CUTTERMODE
                if ((b->MainState   == ACTION_CREATE)  ||
                    (b->MainState   == ACTION_DEAD)    ||
                    (b->MainState   == ACTION_BEAM)    ||
                    (b->BactClassID == BCLID_YPAMISSY))
                {
                    continue;
                };
                #endif

                bpx = (LONG) (b->pos.x / MR.x_aspect);
                bpy = (LONG)-(b->pos.z / MR.y_aspect);

                /*** Selektions-Bereich 16 Pixel! ***/
                if ((abs(bpx - mp_x) <= 8) && (abs(bpy - mp_y) <= 8)) {
                    ywd->FrameFlags |= YWFF_MouseOverBact;
                    ywd->SelBact     = b;
                    /*** Bakterium overrides Sektor! ***/
                    ywd->FrameFlags &= ~YWFF_MouseOverSector;
                    /*** und raus... ***/
                    pass = 2;
                    break;
                };
            };
        };
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yw_DoFinderSelect(struct ypaworld_data *ywd, struct ClickInfo *ci)
/*
**  FUNCTION
**      Darf nur aufgerufen werden, wenn YWFF_MouseOverFinder
**      gesetzt ist!
**
**  CHANGED
**      20-Mar-96   floh    created
**      21-Mar-96   floh    ACTION_CREATE wird explizit ausgefiltert
**      14-Oct-96   floh    ACTION_BEAM wird ausgefiltert
**      09-Apr-97   floh    BUGFIX: Control über Finder brachte
**                          Absturz...
**      03-Jul-97   floh    YWFF_GimmeLeader war ziemlich sinnlos,
**                          oder???
**      28-Oct-97   floh    + man konnte sich noch in einen Commander
**                            im Beamzustand reinschalten
**      08-Dec-97   floh    + die Routine kann jetzt auch den Robo-Bact-Ptr
**                            zurueckliefern!
*/
{
    /*** überhaupt in einem Modus, wo Bakterien interessieren? ***/
    if (ywd->FrameFlags & YWFF_DoBactCheck) {

        LONG item = ci->btn - LV_NUM_STD_BTN;
        struct Bacterium *b = NULL;

        /*** die Einzel-Bakterie ist gefragt... ***/
        b = yw_FRGetBactUnderMouse(ywd, item, ci->act.btnx);

        /*** Treffer? ***/
        if (b) {
            if ((b->MainState != ACTION_CREATE) &&
                (b->MainState != ACTION_DEAD)   &&
                (b->MainState != ACTION_BEAM))
            {
                ywd->FrameFlags |= YWFF_MouseOverBact;
                ywd->SelBact     = b;
            };
        } else {
            /*** kein direkter Treffer -> Commander nehmen ***/
            if (b = FR.b_map[item]) {
                if      (ACTION_CREATE == b->MainState) b=NULL;
                else if (ACTION_BEAM   == b->MainState) b=NULL;
                else if (ACTION_DEAD   == b->MainState) b=NULL;
                if (b) {
                    ywd->FrameFlags |= YWFF_MouseOverBact;
                    ywd->SelBact     = b;
                };
            };
        };
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yw_GetMouseVec(struct ypaworld_data *ywd, struct ClickInfo *ci)
/*
**  FUNCTION
**      Errechnet normalisierten Mouse-Vector, also den Vektor
**      der vom Beobachter durch den Mousezeiger geht.
**
**      Die Routine darf nur aufgerufen werden, wenn ywd->Viewer
**      gültig ist!
**
**  INPUT
**      ywd     -> Ptr auf LID
**      ci      -> ClickInfo-Struktur mit gültigen Mouse-Koords
**
**  RESULT
**      ywd->SelMouseVec enthält den normalisierten Vektor
**
**  CHANGED
**      02-Jan-95   floh    created
**      21-Jul-96   floh    kann jetzt mit Extra-Viewer umgehen.
**      07-Sep-96   floh    sqrt() ersetzt durch nc_sqrt()
*/
{
    FLOAT lpx,lpy;
    FLOAT gpx,gpy,gpz,len;
    struct flt_m3x3 *m;

    m = &(ywd->ViewerDir);

    /*** lokalen Vektor im Viewer-System errechnen ***/
    lpx = ((FLOAT)(ci->act.scrx - (ywd->DspXRes>>1))) / (ywd->DspXRes>>1);
    lpy = ((FLOAT)(ci->act.scry - (ywd->DspYRes>>1))) / (ywd->DspYRes>>1);

    /*** und in die Welt rotieren (lpz ist 1.0) ***/
    gpx = m->m11*lpx + m->m21*lpy + m->m31;
    gpy = m->m12*lpx + m->m22*lpy + m->m32;
    gpz = m->m13*lpx + m->m23*lpy + m->m33;

    /*** Vector normalisieren und nach LID schreiben ***/
    len = nc_sqrt(gpx*gpx+gpy*gpy+gpz*gpz);
    ywd->SelMouseVec.x = gpx/len;
    ywd->SelMouseVec.y = gpy/len;
    ywd->SelMouseVec.z = gpz/len;

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
BOOL yw_FindCreationPoint(struct ypaworld_data *ywd, struct ClickInfo *ci)
/*
**  FUNCTION
**      Sucht einen gültigen Punkt zur Fahrzeug-Erzeugung,
**      ersetzt dabei vollständig "yw_Do3DSelect()" für
**      den Spezialfall "New" und "Add". Im Grunde genommen
**      wird eine "Zielscheibe" von innen nach außen nach
**      einer freien Stelle durchsucht.
**      Bei Erfolg wird ywd->SelMouseVec und ywd->SelDist
**      modifiziert.
**
**  INPUT
**      ywd
**      ci
**
**  RESULTS
**      TRUE    -> alles klaa, out_pos ist gültig
**      FALSE   -> kein Punkt gefunden...
**
**  CHANGED
**      06-Oct-97   floh    created
*/
{
    struct VehicleProto *vp = &(ywd->VP_Array[SR.VPRemap[SR.ActVehicle]]);
    FLOAT r;
    fp3d iv;
    FLOAT l,il;

    /*** ermittle norm. Mouse-Vektor relativ zu Viewer ***/
    iv.x = ((FLOAT)(ci->act.scrx - (ywd->DspXRes>>1))) / (ywd->DspXRes>>1);
    iv.y = ((FLOAT)(ci->act.scry - (ywd->DspYRes>>1))) / (ywd->DspYRes>>1);
    iv.z = 1.0;
    l = nc_sqrt(iv.x*iv.x+iv.y*iv.y+iv.z*iv.z);
    if (l > 0.0) {
        iv.x/=l; iv.y/=l; iv.z/=l;
    };

    /*** Entfernung des Erzeuger-Punktes ***/
    il = GEN_DIST + 4*vp->Radius;

    /*** Öffnungs-Radius in 10 Abschnitten ***/
    for (r=0.0; r<il; r+=(il/10.0)) {
        FLOAT a;    
        fp3d v0;
        struct flt_m3x3 m;

        /*** Ursprungs-Vektor ***/
        v0.x = r;
        v0.y = 0.0;
        v0.z = 0.0;

        /*** eine volle Drehung in 20° Schritten ***/
        for (a=0.0; a<6.283; a+= 0.349) {
            fp3d v1;
            fp3d v2;
            struct flt_m3x3 *vm = &(ywd->ViewerDir);
            struct intersect_msg im;

            /*** v2 ist v1 rotiert entlang des Radius ***/
            m.m11 = cos(a); m.m12=-sin(a); m.m13=0.0;
            m.m21 = sin(a); m.m22=cos(a);  m.m23=0.0;
            m.m31 = 0.0;    m.m32=0.0;     m.m33=1.0;
            v1.x = v0.x*m.m11 + v0.y*m.m12 + v0.z*m.m13;
            v1.y = v0.x*m.m21 + v0.y*m.m22 + v0.z*m.m23;
            v1.z = v0.x*m.m31 + v0.y*m.m32 + v0.z*m.m33;

            /*** verschieben nach relativen "Scheiben-Mittelpunkt" ***/
            v1.x += iv.x*il;
            v1.y += iv.y*il;
            v1.z += iv.z*il;

            /*** und nach Viewer-Orientierung rotieren ***/
            v2.x = v1.x*vm->m11 + v1.y*vm->m21 + v1.z*vm->m31;
            v2.y = v1.x*vm->m12 + v1.y*vm->m22 + v1.z*vm->m32;
            v2.z = v1.x*vm->m13 + v1.y*vm->m23 + v1.z*vm->m33;

            /*** und ein Intersection-Test... ***/
            im.pnt.x = ywd->ViewerPos.x;
            im.pnt.y = ywd->ViewerPos.y;
            im.pnt.z = ywd->ViewerPos.z;
            im.vec.x = v2.x;
            im.vec.y = v2.y;
            im.vec.z = v2.z;
            im.flags = 0;
            _methoda(ywd->world,YWM_INTERSECT2,&im);
            if (!im.insect) {
                /*** alles klaa, v2 normiert zurück ***/
                l = nc_sqrt(v2.x*v2.x + v2.y*v2.y + v2.z*v2.z);
                if (l>0.0) {
                    v2.x/=l; v2.y/=l; v2.z/=l;
                };
                ywd->SelMouseVec = v2;
                ywd->SelDist = l;
                return(TRUE);
            };
        };
    };
    /*** kein gültiger Punkt... ***/
    return(FALSE);
}

/*-----------------------------------------------------------------*/
void yw_Do3DSelect(Object *world, 
                   struct ypaworld_data *ywd, 
                   struct ClickInfo *ci)
/*
**  FUNCTION
**      Wie yw_DoMapSelect(), nur in 3D-Welt...
**
**  CHANGED
**      02-Jan-96   floh    created
**      03-Jan-96   floh    Macht jetzt auch den BactCheck, Welt- und
**                          Bact-Intersection sind jetzt mutual
**                          exclusive.
**      13-Mar-96   floh    Anomalie: der Extra-State-Check beim
**                          Bakterien-Ignorieren war wrong.
**      20-Jun-96   floh    + Flaks werden jetzt außer CONTROL und FIGHT
**                            ignoriert
**      21-Jul-96   floh    ExtraViewer korrigiert
**      07-Sep-96   floh    sqrt() ersetzt durch nc_sqrt()
**      14-Oct-96   floh    + ACTION_BEAM Fahrzeuge werden ignoriert
**      14-Jan-98   floh    + Flaks werden nicht mehr von vorherein
**                            ausgeschlossen
*/
{
    FLOAT check_len = (FLOAT) ywd->NormVisLimit;
    FLOAT sec_dist;
    FLOAT bact_dist;
    struct intersect_msg ismsg;
    struct inbact_msg ibmsg;

    /*** Mouse-Vector bauen (ywd->SelMouseVec) ***/
    yw_GetMouseVec(ywd,ci);

    /*** Intersection-Parameter ermitteln ***/
    ismsg.pnt.x = ibmsg.pnt.x = ywd->ViewerPos.x;
    ismsg.pnt.y = ibmsg.pnt.y = ywd->ViewerPos.y;
    ismsg.pnt.z = ibmsg.pnt.z = ywd->ViewerPos.z;
    ismsg.vec.x = ibmsg.vec.x = ywd->SelMouseVec.x * check_len;
    ismsg.vec.y = ibmsg.vec.y = ywd->SelMouseVec.y * check_len;
    ismsg.vec.z = ibmsg.vec.z = ywd->SelMouseVec.z * check_len;
    ismsg.flags = 0;
    ibmsg.me = ywd->Viewer;

    /*** Intersection gegen Welt und gegen Bakterien ***/
    _methoda(world, YWM_INTERSECT2, &ismsg);
    _methoda(world, YWM_INBACT, &ibmsg);

    /*** falls Intersection gegen Welt... ***/
    if (ismsg.insect) {
        FLOAT ix,iy,iz;

        ywd->FrameFlags |= YWFF_MouseOverSector;
        ix = ismsg.ipnt.x - ywd->ViewerPos.x;
        iy = ismsg.ipnt.y - ywd->ViewerPos.y;
        iz = ismsg.ipnt.z - ywd->ViewerPos.z;

        sec_dist = nc_sqrt(ix*ix + iy*iy + iz*iz);
        ywd->SelDist = sec_dist;
    };

    /*** falls Intersection gegen Bact... ***/
    if (ibmsg.bhit) {
        ywd->FrameFlags |= YWFF_MouseOverBact;
        bact_dist    = ibmsg.dist;
        ywd->SelDist = bact_dist;
    };

    /*** falls Welt- und Bact-Intersection, die nähere nehmen ***/
    if ((ywd->FrameFlags & (YWFF_MouseOverSector|YWFF_MouseOverBact)) ==
        (YWFF_MouseOverSector|YWFF_MouseOverBact))
    {
        if (sec_dist < bact_dist) {
            ywd->FrameFlags &= ~YWFF_MouseOverBact;
            ywd->SelDist    = sec_dist;
        } else {
            ywd->FrameFlags &= ~YWFF_MouseOverSector;
            ywd->SelDist    = bact_dist;
        };
    };

    /*** weitere Parameter ermitteln ***/
    if (ywd->FrameFlags & YWFF_MouseOverSector) {
        LONG secx,secy;

        secx = ((LONG)ismsg.ipnt.x)  / SECTOR_SIZE;
        secy = (-(LONG)ismsg.ipnt.z) / SECTOR_SIZE;

        ywd->SelSector = &(ywd->CellArea[secy*ywd->MapSizeX + secx]);
        ywd->SelSecX   = secx;
        ywd->SelSecY   = secy;

        /*** Intersection-Point ***/
        ywd->SelSecIPos.x = ismsg.ipnt.x;
        ywd->SelSecIPos.y = ismsg.ipnt.y;
        ywd->SelSecIPos.z = ismsg.ipnt.z;

        /*** Sektor-Mittelpunkt ***/
        ywd->SelSecMPos.x = (FLOAT) (secx * SECTOR_SIZE + SECTOR_SIZE/2);
        ywd->SelSecMPos.y = ywd->SelSector->Height;
        ywd->SelSecMPos.z = (FLOAT) -(secy * SECTOR_SIZE + SECTOR_SIZE/2);
    };

    if (ywd->FrameFlags & YWFF_MouseOverBact) {

        /*** die Bact muß bestimmte Anforderungen erfüllen ***/
        struct Bacterium *b = ibmsg.bhit;
        BOOL ignore_me = FALSE;

        /*** Ignoranz-Bedingungen ***/
        #ifndef YPA_CUTTERMODE
        if ((b->MainState   == ACTION_CREATE) ||
            (b->MainState   == ACTION_DEAD)   ||
            (b->MainState   == ACTION_BEAM)   ||
            (b->BactClassID == BCLID_YPAMISSY))
        {
            ignore_me = TRUE;
        };
        #endif
        if (ignore_me) ywd->FrameFlags &= ~YWFF_MouseOverBact;
        else           ywd->SelBact = b;
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yw_MouseSelect(Object *world, 
                    struct ypaworld_data *ywd,
                    struct VFMInput *ip)
/*
**  FUNCTION
**      Findet heraus, ob sich die Maus gerade über einem
**      Sektor oder einem Bakterium in der Map oder
**      der 3D-Ansicht befindet. Wenn ja, werden folgende
**      Daten zurückgeliefert:
**
**      ywd->FrameFlags & YWFF_MouseOverSector,
**      dann:
**
**          ywd->SecX, ywd->SecY ist Sektor-Koordinate
**          ywd->SelSecIPos ist genauer Intersection-Punkt
**          (aber nicht, wenn 2D-Map-Selektion)
**
**      ywd->FrameFlags & YWFF_MouseOverBact,
**      dann:
**
**          ywd->SelBact ist Pointer auf Bacterium
**
**      ywd->SelMouseVec wird ausgefüllt, wenn die Mouse sich
**      nicht über einem Requester befindet.
**
**      Die Routine benötigt die Vorarbeit folgender
**      anderer Routinen:
**
**          yw_maprnd.c/yw_RenderMapInterior()
**          yw_statusbar.c/yw_HandleInputSR()
**
**
**  INPUTS
**      world   -> Ptr auf "mich"
**      ywd     -> Ptr auf LID des Welt-Objects
**      ip      -> Ptr auf VFMInput-Struktur des Frames
**
**  RESULTS
**      ---
**
**  CHANGED
**      29-Dec-95   floh    created
**      20-Mar-96   floh    + YWFF_MouseOverFinder (also Finder-Selektion)
**      23-May-96   floh    + etwas cleverer in bezug auf Radarmodus...
*/
{
    struct ClickInfo *ci = &(ip->ClickInfo);

    /*** ein paar wichtige FrameFlags ermitteln ***/
    yw_PrepareMouseSelect(ywd, ip);

    if (ywd->FrameFlags & YWFF_MouseOverGUI) {
        if (ywd->FrameFlags & YWFF_MouseOverMap) {
            yw_DoMapSelect(ywd, ci);
        } else if (ywd->FrameFlags & YWFF_MouseOverFinder)
            yw_DoFinderSelect(ywd, ci);
    } else {
        yw_Do3DSelect(world, ywd, ci);
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
struct Bacterium *yw_GetCommander(struct Bacterium *b)
/*
**  FUNCTION
**      Ermittelt den Commander zum Bacterium, und gibt
**      Pointer darauf zurück (oder NULL, falls das Bacterium
**      irgendwas ungültiges war).
**
**  CHANGED
**      19-May-96   floh    created
**      27-May-96   floh    Änderungen von AF (Master und Robo in Bact)
*/
{
    if (b) {
        /*** Sonderfälle ausklammern (Robos etc... ***/
        if (((ULONG)b->master) > 2) {
            if (b->master != b->robo) {
                /*** <b> zum Commander machen ***/
                _get(b->master, YBA_Bacterium, &b);
            };
            return(b);
        };
    };

    /*** irgendein Fehler ***/
    return(NULL);
}

/*-----------------------------------------------------------------*/
BOOL yw_SelBact2Cmdr(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Nimmt sich das Vehikel in ywd->SelBact und
**      schreibt den Commander dieses Vehikels nach
**      ywd->SelBact zurück. Falls irgendwas schiefgeht,
**      bleibt ywd->SelBact unverändert und die Routine
**      kommt FALSE zurück.
**
**  CHANGED
**      18-May-96   floh    created
**      
*/
{
    /*** Test, ob SelBact ein Commander ist ***/
    if (ywd->SelBact && (ywd->SelBact != ywd->URBact)) {
        struct Bacterium *sel = yw_GetCommander(ywd->SelBact);
        if (sel) {
            ywd->SelBact = sel;
            return(TRUE);
        };
    };

    return(FALSE);
}

/*-----------------------------------------------------------------*/
void yw_ActionSelect(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Weil die einfache Selektion des Geschwaders, dem
**      ein Vehikel angehört, relativ oft benötigt wird,
**      hier eine dedizierte Routine dafür. Zur Auswahl
**      wird ywd->SelBact verwendet (MUSS gültig sein),
**      modifiziert wird ywd->ActCmdID und ywd->ActCmdr)
**
**  CHANGED
**      30-Dec-95   floh    created
**      18-May-96   floh    Umschalten von SelBact auf seinen
**                          Commander ausgelagert nach yw_SelBact2Cmdr(),
**                          damit andere auch was davon haben.
**      14-Jan-98   floh    Guns werden hier ignoriert. Damit kann man wieder
**                          per Doppelklick in Flaks reinschalten
*/
{
    if (ywd->SelBact->BactClassID != BCLID_YPAGUN) {
        if (ywd->SelBact != ywd->URBact) {
            /*** SelBact umschalten auf seinen Commander ***/
            if (yw_SelBact2Cmdr(ywd)) {
                ywd->ActCmdID = ywd->SelBact->CommandID;
                yw_RemapCommanders(ywd);
            };
        };
    };
}

/*-----------------------------------------------------------------*/
ULONG yw_GenesisCheck(struct ypaworld_data *ywd, struct ClickInfo *ci)
/*
**  FUNCTION
**      Checkt, ob alle Voraussetzungen zum Erschaffen eines
**      neuen Geschwaders erfüllt sind...
**
**  INPUTS
**      ywd - Ptr auf LID des Welt-Objects
**
**  RESULTS
**      0   -> alles OK
**      1   -> zuwenig Platz
**      2   -> zuwenig Energie in Vehikel-Batterie
**      3   -> others
**
**  CHANGED
**      03-Jan-95   floh    created
**      10-Jan-96   floh    ywd->SelDist wird jetzt auf die
**                          günstigste Entfernung gesetzt, so
**                          daß ywd->SelMouseVec * ywd->SelDist
**                          den Genesis-3D-Punkt beschreibt
**      14-Feb-96   floh    Energie-Check aus Milestone2-Version
**                          übernommen
**      08-Apr-96   floh    Energie-Check jetzt mit Hilfe der
**                          Vehikel-Batterie im Robo
**      18-Sep-96   floh    numerische Rückgabewerte, statt BOOL
**      12-Apr-97   floh    + VP_Array nicht mehr global
**      06-Oct-97   floh    + umgebaut für yw_FindCreationPoint()
*/
{
    /*** Mouse nicht über Map! ***/
    if (!(ywd->FrameFlags & YWFF_MouseOverMap)) {

        /*** Ptr auf VehicleProto ***/
        if (SR.ActVehicle != -1) {

            LONG batt_energy;
            LONG needed_energy;
            struct VehicleProto *vp = &(ywd->VP_Array[SR.VPRemap[SR.ActVehicle]]);

            if (!yw_FindCreationPoint(ywd,ci)) return(1);

            /*** genug Energie da? ***/
            needed_energy = CREATE_ENERGY_FACTOR*vp->Energy;
            _get(ywd->UserRobo, YRA_BattVehicle, &batt_energy);
            if (batt_energy < needed_energy) return(2);

            return(0);
        };
    };

    return(3);
}

/*-----------------------------------------------------------------*/
ULONG yw_BuildCheck(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Bauen-Check für User. Folgende Bedingungen müssen
**      erfüllt sein, damit der User einen Sektor bebauen
**      kann:
**          - Sektor ist im eigenen Besitz oder neutral
**          - Sektor ist benachbart
**          - Sektor wird gerade nicht bebaut
**          - Sektor ist nicht der eigene
**
**  INPUTS
**
**  RESULTS
**      0   -> alles OK
**      1   -> Sektor ist ein Spezialsektor, der nicht bebaut werden darf
**      2   -> Owner Error
**      3   -> eigener Sektor
**      4   -> Zu weit weg
**      5   -> wird gerade bebaut
**      6   -> zu wenig Energie
**
**  CHANGED
**      31-Jan-96   floh    created
**      07-Jan-96   floh    Bakterien dürfen jetzt im Sektor sein,
**                          diese werden von YWM_CREATEBUILDING
**                          eh gekillt.
**      10-Feb-96   floh    Bauen auf eigenem Sektor ist disabled.
**      14-Feb-96   floh    Energie-Check aus Milestone2-Version
**                          übernommen.
**      08-Apr-96   floh    Energie-Check jetzt mit Hilfe der
**                          Build-Batterie des Robos
**      09-May-96   floh    Auf vielfachen Wunsch kann man jetzt wieder
**                          in der Map bauen.
**      24-May-96   floh    Im Designer-Modus kann überall gebaut werden.
**      12-Apr-97   floh    + BP_Array nicht mehr global
**      08-Oct-97   floh    + Wundersteine und Beamgates werden
**                            abgewiesen.
**      12-Dec-97   floh    + BattBuilding jetzt BattVehicle
**      26-Feb-98   floh    + auf Superitem darf nicht gebaut werden.
*/
{
    if (ywd->FrameFlags & YWFF_MouseOverSector) {

        #ifndef YPA_DESIGNMODE
        struct Cell *sec = ywd->SelSector;
        struct BuildProto *bp = &(ywd->BP_Array[SR.BPRemap[SR.ActBuilding]]);
        LONG secx = ywd->SelSecX;
        LONG secy = ywd->SelSecY;
        LONG dx,dy;
        LONG batt_energy;

        /*** Owner-Check ***/
        if (sec->Owner != 0) {
            if (sec->Owner != ywd->URBact->Owner) {
                return(2);
            };
        };

        /*** Too Far oder eigener Sektor Check ***/
        dx = abs(secx - ywd->URBact->SectX);
        dy = abs(secy - ywd->URBact->SectY);
        if ((dx == 0) && (dy == 0)) return(3);
        if ((dx > 1) || (dy > 1))   return(4);

        /*** Build-Check ***/
        if (sec->WType == WTYPE_JobLocked) {
            return(5);
        };

        /*** Spezial-Sektor ***/
        if ((WTYPE_Wunderstein == sec->WType) ||
            (WTYPE_ClosedGate == sec->WType)  ||
            (WTYPE_OpenedGate == sec->WType)  ||
            (WTYPE_SuperItem  == sec->WType))

        {
            return(1);
        } else if ((WTYPE_DeadWunderstein==sec->WType) && (ywd->playing_network)) {
            /*** im Netzwerk-Spiel auch eroberten WStein nicht bebauen! ***/
            return(1);
        };

        /*** Energie-Check ***/
        _get(ywd->UserRobo, YRA_BattVehicle, &batt_energy);
        if (batt_energy < bp->CEnergy) {
            return(6);
        };
        #endif

        /*** alle Checks bestanden ***/
        return(0);
    };
    /*** "anderer" Fehler ***/
    return(1);
}

/*-----------------------------------------------------------------*/
ULONG yw_BeamCheck(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Testet eine 3D-Position auf eine gültige
**      Beam-Position. Folgende Bedingungen
**      müssen erfüllt sein:
**
**          - das Gelände muß bekannt sein (Footprint
**            gesetzt).
**          - das Gelände muß im Radius von 200.0
**            eingermaßen eben sein (maximaler
**            Höhenunterschied ca. 200.0).
**          - in der BeamBatterie muß genug
**            Energie vorhanden sein (FIXME!)
**          - die Zielposition muß weiter entfernt
**            sein als 100.0
**
**      Falls Zielsektor ein offenes Beamgate ist,
**      wird kein Energieverbrauch berechnet!
**
**  RESULTS
**      0 -> alles OK (ywd->TLMsg.energy = benötigte Energie!)
**      1 -> zu wenig Energie in BeamBatterie
**      2 -> Gelände zu uneben
**      3 -> unbekanntes Gelände
**      4 -> Ziel ist zu nahe (deckt sich mit 5)
**      5 -> Fahrzeuge im Zielsektor
**      6 -> anderer Fehler
**
**  CHANGED
**      02-Aug-96   floh    created
**      04-Aug-96   floh    benutzt jetzt die Robo-Beam-Batterie
**      20-Aug-96   floh    löscht jetzt zuerst ywd->TLMsg.energy,
**                          damit der Beam-Energy-Balken
**                          nicht durcheinander kommt.
**      22-Aug-96   floh    + Test, ob Fahrzeuge im Zielsektor
**      21-Sep-96   floh    + im Designmode entfallen alle Checks
**      15-Jan-97   floh    + Jaggy-Test entschärft
**                          + Beam-Check schlägt fehl, falls zu
**                            nahe am Welt-Rand
**      28-Feb-97   floh    + der etwas zu konservative Weltrand-
**                            Check (eingeführt, wegen Robo-Guns,
**                            außerhalb der Welt sein konnten -> Absturz)
**                            wurde auf 1/4 Sektor-Größe verändert,
**                            wegen Levels, die nur 1 Sektor breit sind
**      08-Apr-97   floh    + Beamen verhindern, wenn Zeil-Sektor
**                            gerade bebaut wird.
**      25-May-98   floh    + Beamen testweise relaxter gemacht
*/
{
    ywd->TLMsg.energy = 0;
    if (ywd->FrameFlags & YWFF_MouseOverSector) {

        struct Cell *sec = ywd->SelSector;
        FLOAT dx,dy,dz,l;
        LONG e;
        LONG i,j;
        FLOAT max_h,min_h;
        struct intersect_msg imsg;
        LONG beam_batt;
        struct MinList *ls;
        struct MinNode *nd;

        #ifndef YPA_DESIGNMODE
        /*** Footprint-Check ***/
        if (!(sec->FootPrint & (1<<ywd->URBact->Owner))) return(3);

        /*** wird Sektor gerade bebaut??? ***/
        if (WTYPE_JobLocked == sec->WType) return(6);

        /*** Entfernungs-Energie-Berechnung ***/
        dx = ywd->SelSecIPos.x - ywd->URBact->pos.x;
        dy = ywd->SelSecIPos.y - ywd->URBact->pos.y;
        dz = ywd->SelSecIPos.z - ywd->URBact->pos.z;
        l = nc_sqrt(dx*dx+dy*dy+dz*dz);
        if (l < 100.0) return(4);   // zu nahe

        /*** Energieverbrauch ***/
        if ((WTYPE_OpenedGate == sec->WType) ||
            ((WTYPE_Kraftwerk == sec->WType) &&
            (ywd->URBact->Owner == sec->Owner))) {
            /*** Ziel ist offenes Beamgate, oder eigenes Kraftwerk, ***/
            /*** kein Energieverbrauch ***/
            e = 0;
        } else {
            e = (LONG) ((l*l)/230.4);    // 4 Sektoren mit 100.000 KW
        };
        #else
            e = 0;  // YPA_DESIGNMODE
        #endif

        /*** Energieverbrauch zu hoch? (FIXME!) ***/
        _get(ywd->UserRobo, YRA_BattBeam, &beam_batt);
        if (e > beam_batt) return(1);
        else ywd->TLMsg.energy = e;

        /*** LEBENDE Feindrobos im Sektor -> Abbruch ***/
        #ifndef YPA_DESIGNMODE
        ls = (struct MinList *) &(sec->BactList);
        for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {
            struct Bacterium *b = (struct Bacterium *)nd;
            if ((b->BactClassID == BCLID_YPAROBO) &&
                (b->MainState   != ACTION_DEAD)   &&
                (b->Owner != ywd->URBact->Owner))
            {
                return(5);
            };
        };
        #endif

        /*** Gelände-Ebenheits-Check (7*7 Testpunkte, 400.0/7 Abstand) ***/
        min_h = +100000.0;
        max_h = -100000.0;
        imsg.vec.x = 0.0;
        imsg.vec.y = 50000;
        imsg.vec.z = 0.0;
        imsg.flags = 0;
        for (i=-3; i<4; i++) {
            for (j=-3; j<4; j++) {
                imsg.pnt.x = ywd->SelSecIPos.x - i*(400.0/7);
                imsg.pnt.y = -25000;
                imsg.pnt.z = ywd->SelSecIPos.z - j*(400.0/7);
                _methoda(ywd->world, YWM_INTERSECT, &imsg);
                if (imsg.insect) {
                    if (imsg.ipnt.y < min_h) min_h = imsg.ipnt.y;
                    if (imsg.ipnt.y > max_h) max_h = imsg.ipnt.y;
                };
            };
        };
        /*** zu uneben? ***/
        #ifndef YPA_DESIGNMODE
        if (abs(max_h - min_h) > 800.0) return(2);
        #endif

        /*** Weltrand-Check ***/
        if ((ywd->SelSecIPos.x < (SECTOR_SIZE*1.25))  ||
            (ywd->SelSecIPos.z > (-SECTOR_SIZE*1.25)) ||
            (ywd->SelSecIPos.x > (ywd->WorldSizeX-(SECTOR_SIZE*1.25))) ||
            (ywd->SelSecIPos.z < (-ywd->WorldSizeY+(SECTOR_SIZE*1.25))))
        {
            /*** zu nahe am Weltrand (== too jaggy) ***/
            return(2);
        };

        /*** Höhe anpassen und erfolgreich zurück ***/
        ywd->SelSecIPos.y = min_h - ywd->URBact->pref_height;
        return(0);
    };

    /*** anderer Fehler ***/
    return(6);
}

/*-----------------------------------------------------------------*/
ULONG yw_HandleWaypoints(struct ypaworld_data *ywd,
                         struct VFMInput *ip,
                         ULONG action)
/*
**  FUNCTION
**      Handelt alle Aktionen bezüglich Waypoints ab.
**
**  RESULTS
**      Zurückgegeben wird eine neue ACTION (in der Tat werden
**      im Wegpunkt-Modus alle ACTION_GOTOS in WAYPOINT-Actions
**      umgewandelt.
**
**  CHANGED
**      07-Oct-97   floh    created
*/
{
    ULONG ret_action = action;
    struct ClickInfo *ci = &(ip->ClickInfo);
    if (!(ywd->WayPointMode) && (ip->Buttons & BT_WAYPOINT)){

        /*** starte Wegpunkt-Modus ***/
        ywd->WayPointMode = TRUE;
        ywd->NumWayPoints = 0;

    }else if (ywd->WayPointMode && (!(ip->Buttons & BT_WAYPOINT))){

        /*** beende Wegpunkt-Modus ***/
        ywd->WayPointMode = FALSE;

    }else if (ywd->WayPointMode){
        if ((YW_ACTION_GOTO == action) && (ci->flags & CIF_MOUSEDOWN)) {
            if (ywd->NumWayPoints == 0) {
                /*** der allererste Waypoint ***/
                ret_action = YW_ACTION_WAYPOINT_START;
                ywd->FirstWayPoint.x = ywd->SelSecIPos.x;
                ywd->FirstWayPoint.y = ywd->SelSecIPos.y;
                ywd->FirstWayPoint.z = ywd->SelSecIPos.z;
            } else {
                /*** testen, ob aktueller auf 1. Waypoint liegt ***/
                FLOAT x,z,l;
                x = ywd->FirstWayPoint.x - ywd->SelSecIPos.x;
                z = ywd->FirstWayPoint.z - ywd->SelSecIPos.z;
                l = nc_sqrt(x*x + z*z);
                if (l < 100.0) {
                    /*** Cycle-Befehl geben! ***/
                    ret_action = YW_ACTION_WAYPOINT_CYCLE;
                } else {
                    /*** ein normaler neuer Waypoint ***/
                    ret_action = YW_ACTION_WAYPOINT_ADD;
                };
            };
            ywd->NumWayPoints++;
        };
    };
    return(ret_action);
}

/*-----------------------------------------------------------------*/
void yw_RealizeAction(Object *world, struct ypaworld_data *ywd, ULONG action)
/*
**  FUNCTION
**      Führt Reaktion aus Mausklick aus.
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      30-Dec-95   floh    created
**      04-Jan-96   floh    alles fertig außer YW_ACTION_BUILD
**      10-Jan-96   floh    + YW_ACTION_APILOT
**                          + YW_ACTION_PANIC gibt jetzt
**                            Commander des Geschwaders an
**      10-Feb-96   floh    + YW_ACTION_BUILD komplett
**      24-May-96   floh    - Panic-Modus
**      14-Jul-96   floh    + CutterMode-Support
**      22-Jul-96   floh    - YW_ACTION_FIRE gekillt
**      09-Apr-97   floh    + Falls YW_ACTION_CONTROL: Order-Modus aktivieren
**                          + Falls Beam: ORDER
**      15-Aug-97   floh    + falls Fahrzeug gewechselt wird, wird
**                            der DragLock deaktiviert
**      26-Sep-97   floh    + VoiceFeedback bei VehicleChange
**      08-Oct-97   floh    + Waypoint-Handling
**      05-Dec-97   floh    + ACTION_SELECT und ACTION_GOTO aktivieren jetzt 
**                            den Order-Modus
**      08-Dec-97   floh    + Auswaehlen der Hoststation aktiviert jetzt den Beammodus
**      16-Mar-98   floh    + nach Gebäude-Bau-Aktion wird in Order-Modus
**                            zurückgeschaltet
*/
{
    switch (action) {

        case YW_ACTION_SELECT:
            yw_ActionSelect(ywd);
            if (ywd->SelBact == ywd->URBact) {
                SR.ActiveMode = (STAT_MODEF_AUTOPILOT & SR.EnabledModes);
            } else {
                SR.ActiveMode = (STAT_MODEF_ORDER & SR.EnabledModes);
            };
            break;

        case YW_ACTION_GOTO:
            ywd->TLMsg.user_action = YW_ACTION_GOTO;
            ywd->TLMsg.selbact     = ywd->CmdrRemap[ywd->ActCmdr];
            if (ywd->FrameFlags & YWFF_MouseOverSector) {
                ywd->TLMsg.tarsec   = ywd->SelSector;
                ywd->TLMsg.tarsec_x = ywd->SelSecX;
                ywd->TLMsg.tarsec_y = ywd->SelSecY;
                ywd->TLMsg.tarpoint = ywd->SelSecIPos;
                ywd->TLMsg.tarbact  = NULL;
            } else {
                ywd->TLMsg.tarsec   = NULL;
                ywd->TLMsg.tarbact  = ywd->SelBact;
            };
            SR.ActiveMode = (STAT_MODEF_ORDER & SR.EnabledModes);
            break;

        case YW_ACTION_WAYPOINT_START:
        case YW_ACTION_WAYPOINT_ADD:
        case YW_ACTION_WAYPOINT_CYCLE:
            ywd->TLMsg.user_action = action;
            ywd->TLMsg.selbact     = ywd->CmdrRemap[ywd->ActCmdr];
            if (ywd->FrameFlags & (YWFF_MouseOverSector|YWFF_MouseOverBact)) {
                ywd->TLMsg.tarsec   = ywd->SelSector;
                ywd->TLMsg.tarsec_x = ywd->SelSecX;
                ywd->TLMsg.tarsec_y = ywd->SelSecY;
                ywd->TLMsg.tarpoint = ywd->SelSecIPos;
                ywd->TLMsg.tarbact  = NULL;
            };
            break;

        case YW_ACTION_NEW:
            ywd->TLMsg.user_action = YW_ACTION_NEW;
            ywd->TLMsg.proto_id    = SR.VPRemap[SR.ActVehicle];
            ywd->TLMsg.tarpoint.x  = ywd->ViewerPos.x + ywd->SelMouseVec.x * ywd->SelDist;
            ywd->TLMsg.tarpoint.y  = ywd->ViewerPos.y + ywd->SelMouseVec.y * ywd->SelDist;
            ywd->TLMsg.tarpoint.z  = ywd->ViewerPos.z + ywd->SelMouseVec.z * ywd->SelDist;
            SR.ActiveMode = STAT_MODEF_ADD;
            /*** ActCmdr invalidieren (yw_RemapCommanders() wählt dann 1.) ***/
            ywd->ActCmdr  = -1;
            ywd->ActCmdID = 0;
            break;

        case YW_ACTION_ADD:
            ywd->TLMsg.user_action = YW_ACTION_ADD;
            ywd->TLMsg.selbact     = ywd->CmdrRemap[ywd->ActCmdr];
            ywd->TLMsg.proto_id    = SR.VPRemap[SR.ActVehicle];
            ywd->TLMsg.tarpoint.x  = ywd->ViewerPos.x + ywd->SelMouseVec.x * ywd->SelDist;
            ywd->TLMsg.tarpoint.y  = ywd->ViewerPos.y + ywd->SelMouseVec.y * ywd->SelDist;
            ywd->TLMsg.tarpoint.z  = ywd->ViewerPos.z + ywd->SelMouseVec.z * ywd->SelDist;
            break;

        case YW_ACTION_CONTROL:
            {
                yw_ChangeViewer(ywd,ywd->SelBact->BactObject,ywd->Viewer->BactObject);
                yw_ActionSelect(ywd);            
            };
            break;

        case YW_ACTION_BUILD:
            /*** ein Bauen-Auftrag ***/
            ywd->TLMsg.user_action = YW_ACTION_BUILD;
            ywd->TLMsg.proto_id = SR.BPRemap[SR.ActBuilding];
            ywd->TLMsg.tarsec   = ywd->SelSector;
            ywd->TLMsg.tarsec_x = ywd->SelSecX;
            ywd->TLMsg.tarsec_y = ywd->SelSecY;
            ywd->TLMsg.tarpoint = ywd->SelSecIPos;
            ywd->TLMsg.tarbact  = NULL;
            SR.ActiveMode = (STAT_MODEF_ORDER & SR.EnabledModes);
            break;

        case YW_ACTION_APILOT:
            ywd->TLMsg.user_action = YW_ACTION_APILOT;
            ywd->TLMsg.tarsec   = ywd->SelSector;
            ywd->TLMsg.tarsec_x = ywd->SelSecX;
            ywd->TLMsg.tarsec_y = ywd->SelSecY;
            ywd->TLMsg.tarpoint = ywd->SelSecIPos;
            ywd->TLMsg.tarbact  = NULL;
            /*** Order-Modus aktivieren ***/
            SR.ActiveMode = STAT_MODEF_ORDER;
            break;

        #ifdef YPA_DESIGNMODE
        case YW_ACTION_SETSECTOR:
            yw_DesignSetSector(ywd, ywd->SelSector, 
                               ywd->SelSecX, ywd->SelSecY,
                               SR.SecRemap[SR.ActSector]);
            break;

        case YW_ACTION_SETOWNER:
            yw_DesignSetOwner(ywd, ywd->SelSector,
                              ywd->SelSecX, ywd->SelSecY,
                              SR.ActOwner);
            break;

        case YW_ACTION_SETHEIGHT:
            yw_DesignSetHeight(ywd, ywd->SelSector,
                               ywd->SelSecX, ywd->SelSecY,
                               SR.ActHeight);
            break;
        #endif

        #ifdef YPA_CUTTERMODE
        case YW_ACTION_CAMERAPOS:
            ywd->in_seq->cam_pos.x = ywd->SelSecIPos.x;
            ywd->in_seq->cam_pos.y = ywd->SelSecIPos.y - 100;
            ywd->in_seq->cam_pos.z = ywd->SelSecIPos.z;
            break;

        case YW_ACTION_CAMERALINK:
            {
                struct playercontrol_msg pcm;
                pcm.mode = 0;
                switch(SR.ActiveMode) {
                    case STAT_MODEF_LINKSTATIV:
                        pcm.mode = CAMERA_LINKSTATIV; break;
                    case STAT_MODEF_ONBOARD1:
                        pcm.mode = CAMERA_ONBOARD1; break;
                    case STAT_MODEF_ONBOARD2:
                        pcm.mode = CAMERA_ONBOARD2; break;
                };
                pcm.arg0 = ywd->SelBact->ident;
                _methoda(ywd->world, YWM_PLAYERCONTROL, &pcm);
            };
            break;
        #endif

        case YW_ACTION_ONBOARD:
            {
                /*** Onboard-Kamera ***/
                Object *new_viewer, *act_viewer;

                act_viewer = ywd->Viewer->BactObject;
                _set(act_viewer, YBA_Viewer, FALSE);
                _set(act_viewer, YBA_UserInput, FALSE);
                new_viewer = ywd->SelBact->BactObject;
                _set(new_viewer, YBA_Viewer, TRUE);
            };
            break;
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
BOOL yw_CheckDoubleClick(struct ypaworld_data *ywd, struct ClickInfo *ci)
/*
**  FUNCTION
**      Testet auf eine DoubleClick-Situation, mit der die
**      Kontrolle über ein Fahrzeug übernommen werden kann.
**      Sollte IMMER aufgerufen werden, nicht nur bei
**      Mausdown.
**
**  CHANGED
**      03-Jul-97   floh    created
**      19-Mar-98   floh    Testet jetzt nicht mehr, ob beim
**                          2.Klick die Maus noch über derselben
**                          Bakterie hängt, damit wird das Klicken
**                          viel einfacher. Außerdem wurde der
**                          Klickbereich auf 4 Pixel rundrum
**                          erweitert.
**      20-Mar-98   floh    + benutzt jetzt das neue CIF_MOUSEDBLCLCK
**                            Flag, um einen Doppelklick zu erkennen.
*/
{
    if ((ci->flags & CIF_MOUSEDBLCLCK) &&
        (ywd->ClickControlBact) &&
        ((abs(ci->act.scrx - ywd->ClickControlX)) < 4) &&
        ((abs(ci->act.scry - ywd->ClickControlY)) < 4))
    {
        /*** ein gültiger Doubleclick ***/
        ywd->FrameFlags |= YWFF_MouseOverBact;
        ywd->SelBact     = ywd->ClickControlBact;
        ywd->ClickControlTimeStamp = 0;
        ywd->ClickControlBact = NULL;
        ywd->ClickControlX = 0;
        ywd->ClickControlY = 0;
        return(TRUE);
    } else if ((ci->flags & CIF_MOUSEDOWN) &&
               (ywd->FrameFlags & YWFF_MouseOverBact) &&
               (ywd->URBact->Owner == ywd->SelBact->Owner))
    {
        /*** ein gültiger "Erstklick" ***/
        ywd->ClickControlTimeStamp = ywd->TimeStamp;
        ywd->ClickControlBact      = ywd->SelBact;
        ywd->ClickControlX = ci->act.scrx;
        ywd->ClickControlY = ci->act.scry;
    } else if ((ci->flags & CIF_MOUSEDOWN) &&
               (!(ywd->FrameFlags & YWFF_MouseOverBact)))
    {
        /*** ein Klick ins Leere ***/
        ywd->ClickControlTimeStamp = 0;
        ywd->ClickControlBact = NULL;
        ywd->ClickControlX = 0;
        ywd->ClickControlY = 0;
    };
    return(FALSE);
}

/*-----------------------------------------------------------------*/
void yw_BuildTrLogicMsg(Object *world,
                        struct ypaworld_data *ywd, 
                        struct VFMInput *ip)
/*
**  FUNCTION
**      (1) setzt den passenden Mouse-Pointer
**      (2) wenn der User eine Aktion auswählt, wird diese
**          in die TriggerLogic-Message eingebaut, damit die
**          Robos wissen, was los ist.
**
**      Die Routine ruft selbständig yw_MouseSelect() auf.
**
**  INPUTS
**      world   -> Pointer auf "mich"
**      ywd     -> Pointer auf LID des Welt-Objects
**      ip      -> Pointer auf VFMInput-Struktur dieses Frames
**
**  RESULTS
**
**  CHANGED
**      30-Dec-95   floh    created
**      04-Jan-96   floh    Check, ob Viewer vorhanden
**      10-Jan-96   floh    FIGHT arbeitet jetzt auch über Map,
**                          für Fernwaffen
**      21-Mar-96   floh    - MERGE-Modus
**      09-May-96   floh    Im ADD-Modus kann jetzt Selected werden,
**                          wenn Maus über Finder oder Map ist.
**      24-May-96   floh    Panic-Modus removed
**      28-May-96   floh    + Audio-Support
**      29-May-96   floh    + expliziter Select-Sound, zur besseren
**                            Rückmeldung, ob tatsächlich selektiert
**                            wurde.
**      14-Jun-96   floh    + Goto in unbekanntes Gebiet jetzt generell
**                            als ATTACK
**      16-Jun-96   floh    + ControlMode-Handling
**      18-Jun-96   floh    + Mausblanker vorerst wieder entfernt. Da
**                            bedarf es doch einer besseren Lösung.
**      22-Jul-96   floh    + STAT_MODEF_FIGHT-Behandlung ganz raus!
**      02-Aug-96   floh    + STAT_MODEF_AUTOPILOT-Handling für
**                            Beam-Zeugs umgebaut.
**      22-Aug-96   floh    + Support für MouseBlanker
**      18-Sep-96   floh    + Aktions- und Fehler-Tooltips
**      04-Nov-96   floh    + ???
**      12-Dec-96   floh    + Aktionen jetzt NORMALERWEISE wieder per
**                            MOUSEUP-Event, die Überschneidung mit
**                            der Drag-Select-Logik wird durch einen
**                            massiven Hack im HandleInput der Map
**                            erledigt! Das Problem war, daß es
**                            durch das MouseUp zu tonnenweise unbeabsichtigter
**                            Aktivierungen kam (DragWindow, Mousepointer
**                            beim Loslassen über 3D-View -> Aktion).
**      15-Jan-97   floh    + Cleanup Mauspointers.
**      25-Oct-97   floh    + GUISoundCarrier Ereignisse raus
**      08-Dec-97   floh    + falls im Beam-Modus MouseOverBact, wird Select bzw.
**                            Attack aktiviert. Bei der Mapselection muessen dann
**                            Beam-spezifisch die Bacts ignoriert werden...
*/
{
    struct ClickInfo *ci = &(ip->ClickInfo);

    /*** ohne UserRobo geht nix ***/
    if (ywd->URBact && (ci->flags & CIF_VALID) && (ywd->Viewer)) {

        LONG mptr_type;
        ULONG action;
        ULONG result;
        ULONG tip = TOOLTIP_NONE;
        struct disp_pointer_msg dpm;

        /*** ermitteln, ob Mouse über etwas wichtigem ist... ***/
        yw_MouseSelect(world, ywd, ip);

        /*** richtigen Mouse-Pointer ermitteln ***/
        if (ywd->ControlLock) {

            /*** ControlMode: Mouse ausblenden, kein Selecting ***/
            mptr_type = -1;
            action = YW_ACTION_NONE;

        }else if (ywd->DragLock ||
            ((ywd->FrameFlags & YWFF_MouseOverGUI) &&
            (!(ywd->FrameFlags & (YWFF_MouseOverMap|YWFF_MouseOverFinder)))))
        {
            /*** Mouse ist über einem Stino-Requester oder Dragging on... ***/
            mptr_type = YW_MOUSE_POINTER;
            action    = YW_ACTION_NONE;
        }else if (MR.flags & MAPF_DRAGGING) {
            /*** in Map findet gerade ein Drag-Select statt ***/
            mptr_type = YW_MOUSE_SELECT;
            action    = YW_ACTION_NONE;
        }else{

            mptr_type = YW_MOUSE_CANCEL;
            action    = YW_ACTION_NONE;

            switch (SR.ActiveMode) {

                /*----------------------------------------------------*/
                case STAT_MODEF_ORDER:

                    /*** überhaupt jemand da zum befehlen? ***/
                    if (ywd->FrameFlags & YWFF_MouseOverSector) {
                        if (ywd->ActCmdr != -1) {
                            if ((ywd->URBact->Owner == ywd->SelSector->Owner) &&
                                (ywd->SelSector->FootPrint & (1<<ywd->URBact->Owner)))
                            {
                                mptr_type = YW_MOUSE_GOTO;
                                action    = YW_ACTION_GOTO;
                                tip       = TOOLTIP_ACTION_GOTO;
                            } else {
                                mptr_type = YW_MOUSE_ATTACK;
                                action    = YW_ACTION_GOTO;
                                tip       = TOOLTIP_ACTION_ATTACK_SEC;
                            };
                        };

                    } else if (ywd->FrameFlags & YWFF_MouseOverBact) {

                        /*** eigenes Bakterium ***/
                        if (ywd->URBact->Owner == ywd->SelBact->Owner) {
                            mptr_type = YW_MOUSE_SELECT;
                            action    = YW_ACTION_SELECT;
                            tip       = TOOLTIP_ACTION_SELECT;
                        } else {
                            if (ywd->ActCmdr != -1) {
                                mptr_type = YW_MOUSE_ATTACK;
                                action    = YW_ACTION_GOTO;
                                tip       = TOOLTIP_ACTION_ATTACK_VHCL;
                            };
                        };
                    };
                    break;

                /*-----------------------------------------------------*/
                case STAT_MODEF_NEW:
                    if (ywd->FrameFlags & YWFF_MouseOverMap) {
                        /*** in Map funktioniert New-Modus wie Order ***/
                        if (ywd->ActCmdr != -1) {
                            if (ywd->FrameFlags & YWFF_MouseOverSector) {
                                if ((ywd->URBact->Owner == ywd->SelSector->Owner) &&
                                    (ywd->SelSector->FootPrint & (1<<ywd->URBact->Owner)))
                                {
                                    mptr_type = YW_MOUSE_GOTO;
                                    action    = YW_ACTION_GOTO;
                                    tip       = TOOLTIP_ACTION_GOTO;
                                } else {
                                    mptr_type = YW_MOUSE_ATTACK;
                                    action    = YW_ACTION_GOTO;
                                    tip       = TOOLTIP_ACTION_ATTACK_SEC;
                                };
                            } else if (ywd->FrameFlags & YWFF_MouseOverBact) {
                                if (ywd->URBact->Owner == ywd->SelBact->Owner) {
                                    mptr_type = YW_MOUSE_SELECT;
                                    action    = YW_ACTION_SELECT;
                                    tip       = TOOLTIP_ACTION_SELECT;
                                } else {
                                    mptr_type = YW_MOUSE_ATTACK;
                                    action    = YW_ACTION_GOTO;
                                    tip       = TOOLTIP_ACTION_ATTACK_VHCL;
                                };
                            };
                        };
                    } else {
                        if ((ywd->FrameFlags & YWFF_MouseOverBact) &&
                            (ywd->URBact->Owner == ywd->SelBact->Owner))
                        {
                            /*** wenn Pointer ueber Vehikel, Select-Modus ***/
                            mptr_type = YW_MOUSE_SELECT;
                            action    = YW_ACTION_SELECT;
                            tip       = TOOLTIP_ACTION_SELECT;
                        } else {
                            result = yw_GenesisCheck(ywd,ci);
                            switch(result) {
                                case 0:
                                    mptr_type = YW_MOUSE_NEW;
                                    action    = YW_ACTION_NEW;
                                    tip       = TOOLTIP_ACTION_NEW;
                                    break;
                                case 1:
                                    /*** kein Platz ***/
                                    tip = TOOLTIP_ERROR_NOROOM;
                                    break;
                                case 2:
                                    /*** keine Energie ***/
                                    tip = TOOLTIP_ERROR_NOENERGY;
                                    break;
                            };
                        };
                    };
                    break;
                /*-----------------------------------------------------*/
                case STAT_MODEF_ADD:
                    if (ywd->FrameFlags & YWFF_MouseOverMap){
                        /*** in Map funktioniert Add-Modus wie Order ***/
                        if (ywd->ActCmdr != -1) {
                            if (ywd->FrameFlags & YWFF_MouseOverSector) {
                                if ((ywd->URBact->Owner == ywd->SelSector->Owner) &&
                                    (ywd->SelSector->FootPrint & (1<<ywd->URBact->Owner)))
                                {
                                    mptr_type = YW_MOUSE_GOTO;
                                    action    = YW_ACTION_GOTO;
                                    tip       = TOOLTIP_ACTION_GOTO;
                                } else {
                                    mptr_type = YW_MOUSE_ATTACK;
                                    action    = YW_ACTION_GOTO;
                                    tip       = TOOLTIP_ACTION_ATTACK_SEC;
                                };
                            } else if (ywd->FrameFlags & YWFF_MouseOverBact) {
                                if (ywd->URBact->Owner == ywd->SelBact->Owner) {
                                    mptr_type = YW_MOUSE_SELECT;
                                    action    = YW_ACTION_SELECT;
                                    tip       = TOOLTIP_ACTION_SELECT;
                                } else {
                                    mptr_type = YW_MOUSE_ATTACK;
                                    action    = YW_ACTION_GOTO;
                                    tip       = TOOLTIP_ACTION_ATTACK_VHCL;
                                };
                            };
                        };
                    } else {
                        if ((ywd->FrameFlags & YWFF_MouseOverBact) &&
                            (ywd->URBact->Owner == ywd->SelBact->Owner))
                        {
                            /*** wenn Pointer ueber Vehikel, Select-Modus ***/
                            mptr_type = YW_MOUSE_SELECT;
                            action    = YW_ACTION_SELECT;
                            tip       = TOOLTIP_ACTION_SELECT;
                        
                        } else if (ywd->ActCmdr != -1) {
                            result = yw_GenesisCheck(ywd,ci);
                            switch(result) {
                                case 0:
                                    mptr_type = YW_MOUSE_ADD;
                                    action    = YW_ACTION_ADD;
                                    tip       = TOOLTIP_ACTION_ADD;
                                    break;
                                case 1:
                                    /*** kein Platz ***/
                                    tip = TOOLTIP_ERROR_NOROOM;
                                    break;
                                case 2:
                                    /*** keine Energie ***/
                                    tip = TOOLTIP_ERROR_NOENERGY;
                                    break;
                            };
                        };
                    };
                    break;
                /*-----------------------------------------------------*/
                case STAT_MODEF_CONTROL:
                    /*** nur eigene Fahrzeuge können controlled werden ***/
                    mptr_type = YW_MOUSE_CANCEL;
                    if (ywd->FrameFlags & YWFF_MouseOverBact) {
                        if (ip->ContKey == KEYCODE_F7) {
                            /*** Onboard-Kamera (nicht nur User-Vhcls!) ***/
                            mptr_type = YW_MOUSE_CONTROL;
                            action    = YW_ACTION_ONBOARD;
                        } else if (ywd->URBact->Owner == ywd->SelBact->Owner) {
                            mptr_type = YW_MOUSE_CONTROL;
                            action    = YW_ACTION_CONTROL;
                            tip       = TOOLTIP_ACTION_CONTROL;
                        };
                    };
                    break;
                /*-----------------------------------------------------*/
                case STAT_MODEF_BUILD:
                    if (ywd->FrameFlags & YWFF_MouseOverBact) {
                        if (ywd->URBact->Owner == ywd->SelBact->Owner) {
                            mptr_type = YW_MOUSE_SELECT;
                            action    = YW_ACTION_SELECT;
                            tip       = TOOLTIP_ACTION_SELECT;
                        } else {
                            mptr_type = YW_MOUSE_ATTACK;
                            action    = YW_ACTION_GOTO;
                            tip       = TOOLTIP_ACTION_ATTACK_VHCL;
                        };
                    } else {
                        result = yw_BuildCheck(ywd);
                        switch(result) {
                            case 0:
                                mptr_type = YW_MOUSE_BUILD;
                                action    = YW_ACTION_BUILD;
                                tip       = TOOLTIP_ACTION_BUILD;
                                break;
                             case 2:
                                /*** Owner Error ***/
                                tip = TOOLTIP_ERROR_NOTCONQUERED;
                                break;
                             case 3:
                                /*** Too Close ***/
                                tip = TOOLTIP_ERROR_TOOCLOSE;
                                break;
                            case 4:
                                /*** Too Far ***/
                                tip = TOOLTIP_ERROR_TOOFAR;
                                break;
                            case 5:
                                /*** Build Job Locked ***/
                                tip = TOOLTIP_ERROR_JUSTBUILDING;
                                break;
                            case 6:
                                /*** keine Energie ***/
                                tip = TOOLTIP_ERROR_NOENERGY;
                                break;
                        };
                    };
                    break;
                /*-----------------------------------------------------*/
                case STAT_MODEF_AUTOPILOT:
                    if (ywd->FrameFlags & YWFF_MouseOverBact) {
                        if (ywd->URBact->Owner == ywd->SelBact->Owner) {
                            mptr_type = YW_MOUSE_SELECT;
                            action    = YW_ACTION_SELECT;
                            tip       = TOOLTIP_ACTION_SELECT;
                        } else {
                            mptr_type = YW_MOUSE_ATTACK;
                            action    = YW_ACTION_GOTO;
                            tip       = TOOLTIP_ACTION_ATTACK_VHCL;
                        };
                    } else {
                        /*** nur Sektoren ***/
                        result = yw_BeamCheck(ywd);
                        switch(result) {
                            case 0:
                                mptr_type = YW_MOUSE_BEAM;
                                action    = YW_ACTION_APILOT;
                                tip       = TOOLTIP_ACTION_BEAM;
                                break;
                            case 1:
                                /*** zuwenig Energie ***/
                                tip = TOOLTIP_ERROR_NOENERGY;
                                break;
                            case 2:
                                /*** Gelände zu uneben ***/
                                tip = TOOLTIP_ERROR_TOOJAGGY;
                                break;
                            case 3:
                                /*** unbekanntes Gelände ***/
                                tip = TOOLTIP_ERROR_UNKNOWNAREA;
                                break;
                            case 5:
                                /*** Fahrzeuge im Ziel-Sektor ***/
                                tip = TOOLTIP_ERROR_VHCLSINSECTOR;
                                break;
                        };
                    };
                    break;
                /*-----------------------------------------------------*/
                #ifdef YPA_DESIGNMODE
                case STAT_MODEF_SETSECTOR:
                    /*** nur Sektoren ***/
                    if (ywd->FrameFlags & YWFF_MouseOverSector) {
                        mptr_type = YW_MOUSE_POINTER;
                        action    = YW_ACTION_SETSECTOR;
                    };
                    break;
                /*-----------------------------------------------------*/
                case STAT_MODEF_SETOWNER:
                    /*** nur Sektoren ***/
                    if (ywd->FrameFlags & YWFF_MouseOverSector) {
                        mptr_type = YW_MOUSE_POINTER;
                        action    = YW_ACTION_SETOWNER;
                    };
                    break;
                /*-----------------------------------------------------*/
                case STAT_MODEF_SETHEIGHT:
                    /*** nur Sektoren ***/
                    if (ywd->FrameFlags & YWFF_MouseOverSector) {
                        mptr_type = YW_MOUSE_POINTER;
                        action    = YW_ACTION_SETHEIGHT;
                    };
                    break;
                #endif
                /*-----------------------------------------------------*/
                #ifdef YPA_CUTTERMODE
                case STAT_MODEF_FREESTATIV:
                    /*** nur Sektoren ***/
                    if (ywd->FrameFlags & YWFF_MouseOverSector) {
                        mptr_type = YW_MOUSE_POINTER;
                        action    = YW_ACTION_CAMERAPOS;
                    };
                    break;
                /*-----------------------------------------------------*/
                case STAT_MODEF_ONBOARD1:
                case STAT_MODEF_ONBOARD2:
                    /*** Bact: Link ***/
                    if (ywd->FrameFlags & YWFF_MouseOverBact) {
                        mptr_type = YW_MOUSE_POINTER;
                        action    = YW_ACTION_CAMERALINK;
                    };
                    break;
                #endif
                /*-----------------------------------------------------*/
            };

            /*** Waypoint-Handling, <action> filtern ***/
            action = yw_HandleWaypoints(ywd,ip,action);

            /*** Aktion eintragen ***/
            ywd->Action = action;

            /*** Doppelklick über Fahrzeug? reinsetzen! ***/
            if (yw_CheckDoubleClick(ywd,ci)) {
                mptr_type = YW_MOUSE_CONTROL;
                action    = YW_ACTION_CONTROL;
                ci->flags |= CIF_MOUSEDOWN;
            };

            /*** falls Mausclick, die entsprechende Aktion realisieren ***/
            if (ci->flags & CIF_MOUSEDOWN) {
                yw_RealizeAction(world,ywd,action);

                /*** Soundausgabe ***/
                switch(action) {
                    case YW_ACTION_NONE:
                        break;
                    case YW_ACTION_SELECT:
                        break;
                    default:
                        break;
                };
            };

            if ((ci->flags & CIF_MOUSEHOLD) && (action != YW_ACTION_NONE)) {
                mptr_type = YW_MOUSE_POINTER;
            };
        };

        /*** MouseBlanker-Status beachten! ***/
        if (ywd->MouseBlanked) mptr_type = -1;

        /*** <mptr> ist jetzt ein gültiger MousePointer ***/
        if (mptr_type == -1) dpm.pointer = NULL;
        else                 dpm.pointer = ywd->MousePtrBmp[mptr_type];
        switch(mptr_type) {
            case YW_MOUSE_POINTER:  dpm.type=DISP_PTRTYPE_NORMAL; break;
            case YW_MOUSE_CANCEL:   dpm.type=DISP_PTRTYPE_CANCEL; break;
            case YW_MOUSE_SELECT:   dpm.type=DISP_PTRTYPE_SELECT; break;
            case YW_MOUSE_ATTACK:   dpm.type=DISP_PTRTYPE_ATTACK; break;
            case YW_MOUSE_GOTO:     dpm.type=DISP_PTRTYPE_GOTO; break;
            case YW_MOUSE_DISK:     dpm.type=DISP_PTRTYPE_DISK; break;
            case YW_MOUSE_NEW:      dpm.type=DISP_PTRTYPE_NEW; break;
            case YW_MOUSE_ADD:      dpm.type=DISP_PTRTYPE_ADD; break;
            case YW_MOUSE_CONTROL:  dpm.type=DISP_PTRTYPE_CONTROL; break;
            case YW_MOUSE_BEAM:     dpm.type=DISP_PTRTYPE_BEAM; break;
            case YW_MOUSE_BUILD:    dpm.type=DISP_PTRTYPE_BUILD; break;
            default:                dpm.type=DISP_PTRTYPE_NONE; break;
        };
        /*** Sonderfall Dragging im Finder ***/
        if (FR.flags & FRF_Dragging) dpm.type=DISP_PTRTYPE_NORMAL;
        _methoda(ywd->GfxObject,DISPM_SetPointer,&dpm);

        /*** Tooltip anmelden ***/
        if (tip != TOOLTIP_NONE) yw_Tooltip(ywd,tip);
    };

    /*** Ende ***/
}

