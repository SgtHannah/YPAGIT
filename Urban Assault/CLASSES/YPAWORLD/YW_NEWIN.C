/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_newinit.c,v $
**  $Revision: 38.15 $
**  $Date: 1998/01/06 16:24:47 $
**  $Locker:  $
**  $Author: floh $
**
**  Neue Level-Initialisierungs-Funktionen auf Set-Basis
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <exec/types.h>
#include <utility/tagitem.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nucleus/nucleus2.h"
#include "engine/engine.h"
#include "visualstuff/ov_engine.h"

#include "bitmap/ilbmclass.h"
#include "bitmap/displayclass.h"
#include "skeleton/skltclass.h"
#include "baseclass/baseclass.h"
#include "audio/sampleclass.h"
#include "bitmap/winddclass.h"
#include "ypa/yparoboclass.h"
#include "ypa/ypaworldclass.h"
#include "ypa/ypabactclass.h"
#include "ypa/ypaflyerclass.h"
#include "ypa/ypagunclass.h"
#include "ypa/ypacatch.h"

#include "yw_protos.h" 
#include "yw_gsprotos.h"
#include "yw_netprotos.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus 
_extern_use_ov_engine
_extern_use_audio_engine
_extern_use_input_engine

extern unsigned long wdd_DoDirect3D;

/*=================================================================**
**  Profiler Stuff                                                 **
**=================================================================*/

/*-----------------------------------------------------------------*/
void yw_InitProfiler(struct ypaworld_data *ywd)
/*
**  CHANGED
**      09-Jul-97   floh    created
*/
{
    ULONG i;
    ywd->NumFrames = 0;
    for (i=0; i<PROF_NUM_ITEMS; i++) {
        ywd->Profile[i] = 0;
        ywd->ProfMax[i] = 0;
        ywd->ProfMin[i] = 100000;
        ywd->ProfAll[i] = 0;
    };
}

/*-----------------------------------------------------------------*/
void yw_TriggerProfiler(struct ypaworld_data *ywd)
/*
**  CHANGED
**      09-Jul-97   floh    created
*/
{
    ULONG i;
    ywd->NumFrames++;

    /*** die ersten 5 Frames ignorieren ***/
    if (ywd->NumFrames < 5) return;

    /*** Sonderfall Frametime 2 abfangen ***/
    if (ywd->Profile[PROF_FRAME] > 200) ywd->Profile[PROF_FRAME]=0;
    for (i=0; i<PROF_NUM_ITEMS; i++) {
        if (ywd->Profile[i] != 0) {
            if (ywd->ProfMin[i]>ywd->Profile[i]) ywd->ProfMin[i]=ywd->Profile[i];
            if (ywd->ProfMax[i]<ywd->Profile[i]) ywd->ProfMax[i]=ywd->Profile[i];
            ywd->ProfAll[i] += ywd->Profile[i];
        };
    };
}

/*-----------------------------------------------------------------*/
void yw_KillVPSet(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Löscht das ywd->VisProtos-Array
**
**  CHANGED
**      23-Jan-96   floh    created
**      26-Jan-96   floh    es gibt jetzt kein einzelnes
**                          VPSet-Object mehr...
**      27-Sep-96   floh    VisProto-Array jetzt dynamisch allokiert
*/
{
    memset(ywd->VisProtos,0,MAXNUM_VISPROTOS*sizeof(struct VisProto));
}

/*-----------------------------------------------------------------*/
BOOL yw_LoadVPSet(struct ypaworld_data *ywd, Object *vp_family)
/*
**  FUNCTION
**      Isoliert die Child-Objects von <vp_family> (das sind die
**      einzelnen Visprotos) und initialisiert:
**
**      ywd->VisProtos[]
**
**  INPUTS
**      ywd       - Ptr auf LID des Welt-Objects
**      vp_family - Ptr auf Base-Object-Family mit Visprotos
**
**  RESULTS
**      TRUE/FALSE
**
**  CHANGED
**      23-Jan-95   floh    created
**      26-Jan-96   floh    es existiert kein einzelnes VPSet-Object
**                          mehr
*/
{
    struct MinList *ls;
    struct MinNode *nd;
    LONG i;

    /*** Liste der Child-Objects durchackern ***/
    _get(vp_family, BSA_ChildList, &ls);
    for (i=0, nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ, i++) {

        Object *vp = ((struct ObjectNode *)nd)->Object;

        ywd->VisProtos[i].o = vp;

        _set(vp, BSA_VisLimit,    ywd->NormVisLimit);
        _set(vp, BSA_DFadeLength, ywd->NormDFadeLen);
        _get(vp, BSA_TForm, &(ywd->VisProtos[i].tform));

    };

    /*** Ende ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void yw_KillLegoSet(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Deinitialisiert ywd->Legos[].
**
**  CHANGED
**      23-Jan-96   floh    created
**      26-Jan-96   floh    vereinfacht
**      27-Sep-96   floh    Lego-Array jetzt dynamisch allokiert
*/
{
    ULONG i;

    /*** zuerst die Kollisions-Skeletons ***/
    for (i=0; i<MAXNUM_LEGOS; i++) {
        if (ywd->Legos[i].colobj) _dispose(ywd->Legos[i].colobj);
    };

    /*** Legos-Array clean machen ***/
    memset(ywd->Legos, 0, MAXNUM_LEGOS*sizeof(struct Lego));
}

/*-----------------------------------------------------------------*/
BOOL yw_LoadLegoSet(struct ypaworld_data *ywd, APTR sdf, Object *l_family)
/*
**  FUNCTION
**      Isoliert die einzelnen Legos aus der <l_family> und
**      initialisiert:
**         ywd->Legos[]
**
**  INPUTS
**      ywd     - Ptr auf LID des Welt-Objects
**      file    - Ptr auf offenen Set Description File,
**                gestoppt auf Lego-Description-Abschnitt
**      l_family    - Ptr auf Lego-Family
**
**  RESULTS
**      TRUE/FALSE
**
**  CHANGED
**      23-Jan-96   floh    created
**      26-Jan-96   floh    vereinfacht
**      17-Jun-96   floh    Lego-Explosions-FX revised & updated
**      11-Jul-96   floh    + Zeilenlängen-Fix.
**      07-Dec-96   floh    + liest jetzt bis zu <ywd->NumDestFX>
**                            Explosions-Effekte ein
**                          + maximale Länge einer Zeile auf
**                            1024 erhöht
**      24-Feb-98   floh    + Behandlung der alternativen
**                            Kollisions-Skeletons
*/
{
    UBYTE line[1024];
    ULONG i;
    struct MinList *ls;
    struct MinNode *nd;

    /*** Liste der Child-Objects durchackern ***/
    _get(l_family, BSA_ChildList, &ls);
    for (i=0, nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ, i++) {

        Object *l = ((struct ObjectNode *)nd)->Object;

        ywd->Legos[i].lego = l;

        _set(l, BSA_VisLimit, ywd->NormVisLimit);
        _set(l, BSA_DFadeLength, ywd->NormDFadeLen);
        _set(l, BSA_Static, TRUE);
    };

    /*** dann Zeile für Zeile den Set Description File auswerten ***/
    i=0;
    while ((_FGetS(line, sizeof(line), sdf)) && (*line != '>')) {

        UBYTE *dike_out;
        UBYTE *tok, *dummy;
        struct Lego *lego = &(ywd->Legos[i]);

        /*** Sicherheits-Check ***/
        if (i >= MAXNUM_LEGOS) {
            _LogMsg("Too many legos!\n");
            return(FALSE);
        };

        /*** NewLine und Kommentare eliminieren ***/
        if (dike_out = strpbrk(line,";#\n")) *dike_out = 0;

        /*** 1.Keyword -> Lego-Object (ignorieren) ***/
        if (tok = strtok(line, " \t")) {

            /*** gültige Zeile ***/
            ULONG j;

            /*** 2.Keyword ist ColSklt ***/
            if (tok = strtok(NULL, " \t")) {
                Object *sklto;
                if (sklto = _new("sklt.class", RSA_Name, tok, TAG_DONE)) {

                    struct Skeleton *sklt;
                    _get(sklto, SKLA_Skeleton, &sklt);

                    lego->colobj  = sklto;
                    lego->colsklt = sklt;

                } else {
                    _LogMsg("Couldn't load sklt (%s)\n",tok);
                    return(FALSE);
                };
            } else return(FALSE);

            /*** 3.Keyword ist Font-Page-ID ***/
            if (tok = strtok(NULL, " \t")) {
                lego->page = (UBYTE) strtol(tok, &dummy, 0);
            } else return(FALSE);

            /*** 4.Keyword ist Char-Num ***/
            if (tok = strtok(NULL, " \t")) {
                lego->chr = (UBYTE) strtol(tok, &dummy, 0);
            } else return(FALSE);

            /*** 5.Keyword ist Shield ***/
            if (tok = strtok(NULL, " \t")) {
                lego->shield = (UBYTE) strtol(tok, &dummy, 0);
            } else return(FALSE);

            /*** 6.Keyword ist Alt-Sklt-ID ***/
            if (tok = strtok(NULL, " \t")) {
                ULONG id = strtol(tok, &dummy, 0);
                switch(id) {
                    case 1:
                        /*** Subsektor-Coll-Sklt nehmen ***/
                        lego->altsklt = ywd->AltCollSubSklt;
                        break;
                    case 2:
                        /*** Kompaktsektor-Coll-Sklt nehmen ***/
                        lego->altsklt = ywd->AltCollCompactSklt;
                        break;
                    default:
                        /*** Default-Coll-Sklt nehmen ***/
                        lego->altsklt = lego->colsklt;
                        break;
                };
            }else return(FALSE);

            /*** optional bis zu 8 Elemente folgenden Aufbaus:         ***/
            /*** [vproto x y z] (ohne Eckklammern!) für FX-Explosionen ***/
            j = 0;
            while ((tok = strtok(NULL, " \t")) && (j<ywd->NumDestFX)) {

                FLOAT x = 0.0;
                FLOAT y = 0.0;
                FLOAT z = 0.0;

                lego->fx_vp[j] = strtol(tok,NULL,0);

                if (tok = strtok(NULL, " \t")) x=atof(tok);
                if (tok = strtok(NULL, " \t")) y=atof(tok);
                if (tok = strtok(NULL, " \t")) z=atof(tok);

                /*** Koordinaten eintragen ***/
                lego->fx_pos[j].x = x;
                lego->fx_pos[j].y = -z;
                lego->fx_pos[j].z = -y;

                j++;
            };

            /*** Lego-Index inkrementieren ***/
            i++;
        };
    };

    /*** Ende ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
BOOL yw_ReadSubSectorTypes(struct ypaworld_data *ywd, APTR sdf)
/*
**  FUNCTION
**      Initialisiert
**
**          ywd->SubSects[]
**
**  CHANGED
**      24-Jan-96   floh    übernommen + modifiziert aus altem 
**                          yw_initlevel.c
*/
{
    UBYTE line[255];
    ULONG i = 0;
    UBYTE *tok, *dummy;

    /*** Zeilen lesen... ***/
    while ((_FGetS(line, sizeof(line), sdf)) && (*line != '>')) {

        /*** NewLine eliminieren ***/
        UBYTE *dike_out;

        /*** Sicherheits-Check ***/
        if (i >= MAXNUM_SUBSECTS) {
            _LogMsg("Too many sub sector defs!\n");
            return(FALSE);
        };

        /*** Kommentare und NewLine eliminieren ***/
        if (dike_out = strpbrk(line, ";#\n")) *dike_out = 0;

        /*** 1.Token -> DLINDEX_FULL ***/
        if (tok = strtok(line, " \t")) {

            struct SubSectorDesc *subs = &(ywd->SubSects[i]);

            subs->limit_lego[DLINDEX_FULL] = strtol(tok, &dummy, 0);

            /*** 2.Token -> DLINDEX_DAMAGED ***/
            if (tok = strtok(NULL, " \t")) {
                subs->limit_lego[DLINDEX_DAMAGED] = strtol(tok, &dummy, 0);
            } else return(FALSE);

            /*** 3.Token -> DLINDEX_DESTROYED ***/
            if (tok = strtok(NULL, " \t")) {
                subs->limit_lego[DLINDEX_DESTROYED] = strtol(tok, &dummy, 0);
            } else return(FALSE);

            /*** 4.Token -> DLINDEX_EMPTY ***/
            if (tok = strtok(NULL, " \t")) {
                subs->limit_lego[DLINDEX_EMPTY] = strtol(tok, &dummy, 0);
            } else return(FALSE);

            /*** 5.Token -> Anfangs-Energie-Code (0->Empty, 1->Full) ***/
            if (tok = strtok(NULL, " \t")) {
                switch (*tok) {
                    case '0':
                        subs->init_energy = 0;
                        break;

                    case '1':
                        subs->init_energy = 255;
                        break;

                    default:
                        subs->init_energy = 0;
                        break;
                };
            } else return(FALSE);

            /*** 6.Token -> VisProto-Nummer Reparier-Hülle (optional) ***/
            if (tok = strtok(NULL, " \t")) {
                subs->repair_proto = strtol(tok, &dummy, 0);
            };

            /*** das war eben eine gültige Zeile -> Index++ ***/
            i++;
        };
    };

    /*** Ende ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void yw_ReadError(struct ypaworld_data *ywd, UBYTE *file, UBYTE *line)
/*
**  FUNCTION
**      Gibt einen Lese-Fehler aus, mit der fehlerhaften Zeile.
**
**  CHANGED
**      07-Feb-98   floh    created
*/
{
    _LogMsg("Error reading '%s', line '%s'.\n",file,line);
}

/*-----------------------------------------------------------------*/
BOOL yw_ReadSectorTypes(struct ypaworld_data *ywd, APTR sdf)
/*
**  FUNCTION
**      Initialisiert
**
**          ywd->Sectors[]
**
**  CHANGED
**      24-Jan-96   floh    übernommen aus altem yw_initlevel.c
**      30-Jan-96   floh    macht jetzt am Ende ein paar Checks
**                          auf das BuildProto-Array (z.B. ob 
**                          Kraftwerke einen Compact-Sektor haben)
**      30-Jul-96   floh    Der "Kraftwerk-Ist-Kein-Kompakt-Sektor"
**                          Fehler wurde in eine Warnung umgewandelt,
**                          außerdem war in der Fehlermeldung ein
**                          Fehler
**      12-Apr-97   floh    + BP_Array nicht mehr global
*/
{
    UBYTE line[512];
    UBYTE line_bu[512];
    ULONG i;
    UBYTE *tok, *dummy;

    /*** Zeilen lesen... ***/
    i = 0;
    while ((_FGetS(line, sizeof(line), sdf)) && (*line != '>')) {

        UBYTE *dike_out;

        /*** Backup anlegen ***/
        strcpy(line_bu,line);

        /*** Sicherheits-Check ***/
        if (i >= MAXNUM_SECTORS) {
            _LogMsg("Too many sector defs!\n");
            return(FALSE);
        };

        /*** Kommentare und NewLine eliminieren ***/
        if (dike_out = strpbrk(line, ";#\n")) *dike_out = 0;

        /*** 1.Token -> Sector-Nummer ***/
        if (tok = strtok(line, " \t")) {

            struct SectorDesc *sd;
            ULONG i;    // Sector-Nummer

            i  = strtol(tok, &dummy, 0);
            sd = &(ywd->Sectors[i]);

            /*** 2.Token -> Sektor-Typ (1x1 oder 3x3) ***/
            if (tok = strtok(NULL, " \t")) {
                sd->SecType = strtol(tok, &dummy, 0);
            } else {
                yw_ReadError(ywd,"set.sdf",line_bu);
                return(FALSE);
            };

            /*** 3.Token -> GroundType ***/
            if (tok = strtok(NULL, " \t")) {
                sd->GroundType = strtol(tok, &dummy, 0);
            } else {
                yw_ReadError(ywd,"set.sdf",line_bu);
                return(FALSE);
            };

            /*** 4.Token -> FontChar in SEC8X8 und SEC4X4 ***/
            if (tok = strtok(NULL, " \t")) {
                sd->Chr = strtol(tok, &dummy, 0);
            } else {
                yw_ReadError(ywd,"set.sdf",line_bu);
                return(FALSE);
            };

            /*** ab 5.Token -> 1 oder 9 Sub-Sektor-Nummer(n) ***/
            memset(&(sd->SSD),0,9*sizeof(APTR));
            if (sd->SecType == SECTYPE_COMPACT) {

                /*** nur eine SubSector-Nummer lesen ***/
                if (tok = strtok(NULL, " \t")) {
                    ULONG num = strtol(tok, &dummy, 0);
                    sd->SSD[0][0] = &(ywd->SubSects[num]);
                } else {
                    yw_ReadError(ywd,"set.sdf",line_bu);
                    return(FALSE);
                };

            } else {

                /*** sonst 9 Nummern lesen ***/
                LONG k,j;
                for (j=2; j>=0; j--) {
                    for (k=0; k<=2; k++) {
                        if (tok = strtok(NULL, " \t")) {
                            LONG num = strtol(tok, &dummy, 0);
                            sd->SSD[k][j] = &(ywd->SubSects[num]);
                        } else {
                            yw_ReadError(ywd,"set.sdf",line_bu);
                            return(FALSE);
                        };
                    };
                };
            };
        };
    };

    /*** Ende ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void yw_KillSlurpSet(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Gegenstück zu yw_LoadSlurpSet().
**
**  CHANGED
**      24-Jan-96   floh    created
**      26-Jan-96   floh    vereinfacht
*/
{
    memset(ywd->HSlurp,0,sizeof(ywd->HSlurp));
    memset(ywd->VSlurp,0,sizeof(ywd->VSlurp));

    if (ywd->SideSlurp.o) {
        _dispose(ywd->SideSlurp.o);
        memset(&(ywd->SideSlurp),0,sizeof(ywd->SideSlurp));
    };

    if (ywd->CrossSlurp.o) {
        _dispose(ywd->CrossSlurp.o);
        memset(&(ywd->CrossSlurp),0,sizeof(ywd->CrossSlurp));
    };
}

/*-----------------------------------------------------------------*/
BOOL yw_LoadSlurpSet(struct ypaworld_data *ywd, Object *sl_family)
/*
**  FUNCTION
**      Lädt ein Slurp-Set als Fat Base Object.
**
**  INPUTS
**      ywd         - Ptr auf LID des Welt-Objects
**      sl_family   - Ptr auf Slurp-Object-Family.
**
**  CHANGED
**      24-Jan-96   floh    created
**      26-Jan-96   floh    vereinfacht
**      28-Jan-96   floh    Ooops, statt den BaseObjects
**                          wurden die eingebetteten Skeleton-
**                          Objects als Slurp-Objects eingetragen!
**      25-Feb-98   floh    + die Slurp-Kollissions-Skeletons werden
**                            jetzt aus mc2res/skeleton geladen
*/
{
    ULONG i,j,dir;
    Object *sklto;
    struct Skeleton *sklt;

    struct MinList *ls;
    struct MinNode *nd;

    UBYTE old_path[255];

    /*** die Childs des Fat Objects auseinanderwuseln ***/
    _get(sl_family, BSA_ChildList, &ls);
    nd = ls->mlh_Head;  // erstes Child

    /*** und los... ***/
    for (dir=0; dir<2; dir++) {
        for (i=0; i<NUM_GTYPES; i++) {
            for (j=0; j<NUM_GTYPES; j++) {

                Object *bo;

                if (nd->mln_Succ == NULL) {
                    _LogMsg("Too few slurps in slurp child.\n");
                    return(FALSE);
                };

                /*** Object aus linearer Liste holen und init... ***/
                bo = ((struct ObjectNode *)nd)->Object;

                _set(bo, BSA_VisLimit, ywd->NormVisLimit);
                _set(bo, BSA_DFadeLength, ywd->NormDFadeLen);
                _set(bo, BSA_Static, TRUE);
                _get(bo, BSA_Skeleton, &sklto);
                _get(sklto, SKLA_Skeleton, &sklt);

                /*** Slurp-Stuff eintragen ***/
                switch(dir) {
                    case 0:
                        ywd->HSlurp[i][j].o    = bo;
                        ywd->HSlurp[i][j].sklt = sklt;
                        break;

                    case 1:
                        ywd->VSlurp[i][j].o    = bo;
                        ywd->VSlurp[i][j].sklt = sklt;
                        break;
                };

                /*** nächstes Slurp-Object ***/
                nd = nd->mln_Succ;
            };
        };
    };

    /*** Kollisions-Slurps erzeugen ***/
    strcpy(old_path, _GetAssign("rsrc"));
    _SetAssign("rsrc","data:mc2res");
    sklto = _new("sklt.class", RSA_Name, "Skeleton/ColSide.sklt", TAG_DONE);
    if (sklto) {
        ywd->SideSlurp.o = sklto;
        _get(sklto, SKLA_Skeleton, &(ywd->SideSlurp.sklt));
    } else {
        _LogMsg("Couldn't create side collision sklt.\n");
        return(FALSE);
    };
    sklto = _new("sklt.class", RA_Name, "Skeleton/ColCross.sklt", TAG_DONE);
    if (sklto) {
        ywd->CrossSlurp.o = sklto;
        _get(sklto, SKLA_Skeleton, &(ywd->CrossSlurp.sklt));
    } else {
        _LogMsg("Couldn't create cross collision sklt.\n");
        return(FALSE);
    };
    _SetAssign("rsrc",old_path);

    /*** alles ok ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void yw_KillSet(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Killt das momentan eingestellte Set vollständig:
**
**          TracyRemap
**          ShadeRemap
**          die Fonts
**          Slurps
**          Legos
**          VisProtos
**          SoundSamples in den Prototype-Arrays (along with
**          den anderen setspezifischen Attributen)
**          das Set-Object
**
**  CHANGED
**      24-Jan-96   floh    created
**      26-Jan-96   floh    Jetzt existiert nur noch ein einziges
**                          großes Set-Object, welches VisProtos,
**                          Slurps und Legos einbettet.
**      27-Sep-96   floh    ywd->SubSects und ywd->Sectors jetzt
**                          dynamisch allokiert
**      18-Oct-96   floh    Fix: ywd->SetObject Pointer wird nach
**                          _dispose() auf NULL gesetzt
**      03-Apr-97   floh    + DISPM_EndSession
**      24-Feb-98   floh    + alternative Kollisionsskeletons
**      13-Mar-98   floh    + Maus wird gekillt
**      29-May-98   floh    + Set-Object wird nicht mehr gekillt
**                          + Set-Object wird dorch wieder gekillt...
*/
{
    Object *gfxo;

    yw_KillVPSet(ywd);      // VisProtos
    yw_KillLegoSet(ywd);    // Legos
    yw_KillSlurpSet(ywd);   // Slurps

    /*** SubSec- und Sector-Arrays cleanen ***/
    memset(ywd->SubSects,0,MAXNUM_SUBSECTS*sizeof(struct SubSectorDesc));
    memset(ywd->Sectors,0,MAXNUM_SECTORS*sizeof(struct SectorDesc));

    /*** BeeBox-Object ***/
    if (ywd->BeeBox) {
        _dispose(ywd->BeeBox);
        ywd->BeeBox = NULL;
    };

    /*** Alternative Kollisions-Skeletons ***/
    if (ywd->AltCollSubObject) {
        _dispose(ywd->AltCollSubObject);
        ywd->AltCollSubObject = NULL;
        ywd->AltCollSubSklt   = NULL;
    };
    if (ywd->AltCollCompactObject) {
        _dispose(ywd->AltCollCompactObject);
        ywd->AltCollCompactObject = NULL;
        ywd->AltCollCompactSklt   = NULL;
    };

    /*** Remap-Tables ***/
    if (ywd->TracyRemap) {
        _dispose(ywd->TracyRemap);
        ywd->TracyRemap = NULL;
    };
    if (ywd->ShadeRemap) {
        _dispose(ywd->ShadeRemap);
        ywd->ShadeRemap = NULL;
    };

    /*** Fonts killen ***/
    yw_KillFontSet(ywd);
    
    /*** Set Object killen ***/
    if (ywd->SetObject) {
        _dispose(ywd->SetObject);
        ywd->SetObject = NULL;
        ywd->ActSet = 0;
    };        

    /*** DISPM_EndSession ***/
    _OVE_GetAttrs(OVET_Object,&gfxo,TAG_DONE);
    _methoda(gfxo,DISPM_EndSession,NULL);
    yw_KillMouse(ywd);
}

/*-----------------------------------------------------------------*/
Object *yw_LoadFromList(UBYTE *list_name)
/*
**  FUNCTION
**      Erzeugt eine Object-Collection, indem zuerst
**      ein Master-Object erzeugt wird, dann wird
**      der per <list_name> definierte File gescannt,
**      und das 1.Wort jeder Zeile als Object-File-Name
**      interpretiert. Diese Objects werden geladen,
**      an das Master-Object gehängt, dieses wird zurück-
**      gegeben.
**
**      Die Routine wird als letzter Versuch angewendet,
**      wenn eines der Fat Base Objects nicht gefunden wurde.
**
**  INPUTS
**      list_name   - Name des Listen-Files
**
**  RESULTS
**      Object *, Pointer auf komplettes Collection-Object,
**      oder NULL bei Mißerfolg
**
**  CHANGED
**      13-Mar-96   floh    created
**      19-Mar-96   floh    gibt jetzt bissel detailiertere Fehler-Meldungen
**                          aus...
**      20-Mar-96   floh    Bugfix: nach '>' wurde nicht abgebrochen, so
**                          daß der Set Description File falsch interpretiert
**                          wurde.
**      17-Jul-96   floh    Linebuffer auf 512 Zeichen vergrößert
**      23-Mar-98   floh    + kein extra Setpath-Parameter mehr.
*/
{
    Object *master;
    UBYTE str[256];

    /*** erzeuge Master-Object ***/
    master = _new("base.class", TAG_DONE);
    if (master) {

        APTR file;

        /*** öffne Listen-File ***/
        file = _FOpen(list_name,"r");
        if (file) {

            UBYTE act_line[512];

            /*** lese eine Zeile nach der anderen ***/
            while(_FGetS(act_line, sizeof(act_line), file)) {

                UBYTE *name;

                /*** nur 1.Wort interessiert ***/
                name = strtok(act_line, " #;\t\n");
                if (name && (name[0] != '>')) {

                    Object *child;

                    /*** dieses Object laden ***/
                    strcpy(str, "rsrc:objects/");
                    strcat(str, name);
                    child = _load(str);
                    if (child) _method(master, BSM_ADDCHILD, (ULONG) child);
                    else {
                        /*** Fehler, Einzelobjekt nicht geladen ***/
                        _LogMsg("init: Could not load %s.\n", str);
                        _FClose(file);
                        _dispose(master);
                        return(NULL);
                    };
                } else break;
            };
            _FClose(file);
        } else {
            /*** Fehler, List-File nicht gefunden ***/
            _dispose(master);
            return(NULL);
        };
    };

    /*** Ende ***/
    return(master);
}

/*-----------------------------------------------------------------*/
Object *yw_LoadSetObject(void)
/*
**  FUNCTION
**      Versucht ein komplettes Set-Objects zu erzeugen,
**      dabei geht die Routine etwas intelligenter vor:
**
**      1) versuche, objects/set.base zu laden, schlägt dies
**         fehl, dann (2)
**      2) lade hintereinander visproto.base,
**                             lego.base
**                             slurp.base
**         und bastle daraus ein "Fake" Set-Object.
**
**  INPUTS
**      set_path    - Pfad zum Set, z.B. "set1:"
**
**  RESULTS
**      Pointer auf Object, oder NULL.
**
**  CHANGED
**      08-Feb-96   floh    created
**      13-Mar-96   floh    falls das Laden eines "Sub-Fat-Base-Objects"
**                          auch mißlingt, wird versucht, die Einzel-Objects
**                          anhand der Files 
**                              "scripts/set.sdf",
**                              "scripts/slurps.lst" und/oder
**                              "scripts/visproto.lst" zu laden.
**      23-Mar-98   floh    "rsrc:" Assign muß korrekt auf Set gebogen sein!
*/
{
    Object *set;

    /*** zuerst, set.base versuchen ***/
    set = _load("rsrc:objects/set.base");
    if (NULL == set) {

        _LogMsg("init: no set.base, trying fragment load.\n");

        /*** Laden mißlungen, also die Einzel-Objects versuchen ***/
        set = _new("base.class", TAG_DONE);
        if (set) {

            Object *vp    = NULL;
            Object *lego  = NULL;
            Object *slurp = NULL;

            /*** visproto.base ***/
            vp = _load("rsrc:objects/visproto.base");
            if (!vp) {
                _LogMsg("init: no visproto.base, trying single load.\n");
                vp = yw_LoadFromList("rsrc:scripts/visproto.lst");
            };
            if (vp)  _method(set, BSM_ADDCHILD, (ULONG)vp);
            else {
                _dispose(set);
                return(NULL);
            };

            /*** lego.base ***/
            lego = _load("rsrc:objects/lego.base");
            if (!lego) {
                _LogMsg("init: no lego.base, trying single load.\n");
                lego = yw_LoadFromList("rsrc:scripts/set.sdf");
            };
            if (lego)  _method(set, BSM_ADDCHILD, (ULONG)lego);
            else {
                _dispose(set);
                return(NULL);
            };

            /*** slurp.base ***/
            slurp = _load("rsrc:objects/slurp.base");
            if (!slurp) {
                _LogMsg("init: no slurp.base, trying single load.\n");
                slurp = yw_LoadFromList("rsrc:scripts/slurps.lst");
            };
            if (slurp)  _method(set, BSM_ADDCHILD, (ULONG)slurp);
            else {
                _dispose(set);
                return(NULL);
            };
        };
    };

    /*** Ende ***/
    return(set);
}

/*-----------------------------------------------------------------*/
BOOL yw_LoadSet(struct ypaworld_data *ywd, ULONG set_num)
/*
**  FUNCTION
**      Lädt ein komplettes neues Set.
**
**  INPUTS
**      ywd         - Ptr auf LID des Welt-Objects
**      set_num     - Nummer des gewünschten Sets
**                    [1..n], 0 ist nicht erlaubt, weil
**                    das die ID für "kein Set geladen"
**                    ist.
**
**  RESULTS
**      TRUE/FALSE
**
**  CHANGED
**      24-Jan-96   floh    created
**      26-Jan-96   floh    + statt einzelner Slurp-, Visproto- und
**                            Lego-Fat-Objects gibts jetzt nur noch
**                            ein Mega-Fat-Object
**                          + der SYSPATH_RESOURCE Pfad wird
**                            jetzt immer umgebogen, egal ob das
**                            Set bereits geladen ist oder nicht
**      13-Mar-96   floh    revised & updated
**      23-Apr-96   floh    - Checkt *nicht* mehr, ob das selbe
**                            Set bereits geladen ist. Macht auch
**                            *kein* yw_KillSet() mehr! Das wird
**                            in YWM_KILLLEVEL abgehandelt.
**      26-Apr-96   floh    + Lädt jetzt Standard-Palette des Sets.
**                            (aus setn:palette/standard.pal).
**                          - SYSPATH_RESOURCES wird jetzt früher in
**                            YWM_NEWLEVEL eingestellt.
**      12-Jun-96   floh    + Standard-Farbpalette ohne <path_prefix>
**      26-Feb-97   floh    + Default-Farbpalette wird jetzt vor
**                            dem Laden des SetObjects geladen, damit
**                            auf Systemen mit Custom-Texturen eine
**                            gültige globale Palette existiert, bevor
**                            die erste Textur geladen wird.
**      03-Apr-97   floh    + DISPM_BeginSession
**      24-Feb-98   floh    + alternative Kollisionsskeletons
**      13-Mar-98   floh    + Maus wird initialisiert
**      25-May-98   floh    + Maus wird jetzt am Ende der Routine initialisiert,
**                            damit irgendwelche Warte-Pointer nicht 
**                            ueberschrieben werden.
**      29-May-98   floh    + jetzt mit Load-Optimierung beim Set-Object
**      02-Jun-98   floh    + loescht das ControlLock Flag
**      20-Jun-98   floh    + laedt jetzt nur noch das Font-Set, wenn
**                            es sich um Set46 oder 42(???) handelt, andernfalls
**                            wird es in yw_CommonLevelInit() initialisiert
*/
{
    UBYTE *str;
    UBYTE old_path[256];
    UBYTE *set_path[256];
    struct VFMBitmap *tracy_rmp;
    struct VFMBitmap *shade_rmp;
    ULONG res;
    struct MinList *ls;
    struct MinNode *nd;
    ULONG j;
    APTR sdf;

    _OVE_GetAttrs(OVET_Object,&(ywd->GfxObject),TAG_DONE);

    /*** ControlLock Flag loeschen ***/
    ywd->ControlLock = FALSE;

    /*** Set-Assign setzen ***/
    sprintf(set_path,"data:set%d",set_num);

    /*** Set-Pfad holen ***/
    strcpy(old_path, _GetAssign("rsrc"));

    /*** DISPM_BeginSession ***/
    _methoda(ywd->GfxObject,DISPM_BeginSession,NULL);

    /*** alternative Kollissionsskeletons ***/
    _SetAssign("rsrc", "data:mc2res");
    ywd->AltCollSubObject = _new("sklt.class",RSA_Name,"skeleton/colsub.sklt",TAG_DONE);
    if (ywd->AltCollSubObject) {
        _get(ywd->AltCollSubObject,SKLA_Skeleton,&(ywd->AltCollSubSklt));
    }else{
        _LogMsg("Couldn't load <skeleton/colsub.sklt>, set %d.\n",set_num);
        return(FALSE);
    };
    ywd->AltCollCompactObject = _new("sklt.class",RSA_Name,"skeleton/colcomp.sklt",TAG_DONE);
    if (ywd->AltCollCompactObject) {
        _get(ywd->AltCollCompactObject,SKLA_Skeleton,&(ywd->AltCollCompactSklt));
    }else{
        _LogMsg("Couldn't load <skeleton/colcomp.sklt>, set %d.\n",set_num);
        return(FALSE);
    };

    /*** Set-Object bei Bedarf laden ***/
    _SetAssign("rsrc",set_path);
    if (!_LoadColorMap("palette/standard.pal")) {
        _LogMsg("WARNING: Could not load set default palette!\n");
    };
    
    /*** Set-Object bei Bedarf laden ***/
    if ((set_num != ywd->ActSet) && (set_num != 46)) {
        if (ywd->SetObject) {
            _LogMsg("yw_LoadSet(): killing set object %d\n",ywd->ActSet);
            _dispose(ywd->SetObject);
            ywd->SetObject = NULL;
            ywd->ActSet    = 0;
        };
        if (ywd->SetObject = yw_LoadSetObject()) {
            ywd->ActSet = set_num;
            _LogMsg("yw_LoadSet(): loaded set object %d ok\n",set_num);
        } else {
            ywd->ActSet = 0;
            _LogMsg("yw_LoadSet(): loading set object %d failed\n",set_num);
            _SetAssign("rsrc",old_path);
            return(FALSE);
        };
    };
    
    /*** Set-Description-File parsen ***/
    if (set_num != 46) {
        if (sdf = _FOpen("rsrc:scripts/set.sdf", "r")) {
            _get(ywd->SetObject, BSA_ChildList, &ls);
            for (j=0,nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ,j++) {
                Object *child = ((struct ObjectNode *)nd)->Object;
                switch(j) {
                    case 0:
                        /*** visproto.base ***/
                        if (!yw_LoadVPSet(ywd,child)) return(FALSE);
                        break;

                    case 1:
                        /*** lego.base ***/
                        if (!yw_LoadLegoSet(ywd,sdf,child))  return(FALSE);
                        if (!yw_ReadSubSectorTypes(ywd,sdf)) return(FALSE);
                        if (!yw_ReadSectorTypes(ywd,sdf))    return(FALSE);
                        break;

                    case 2:
                        /*** slurp.base ***/
                        if (!yw_LoadSlurpSet(ywd,child)) return(FALSE);
                        break;
                };
            };
            _FClose(sdf);
        } else {
            _LogMsg("Couldn't open set description file, set %d!\n",set_num);
            return(FALSE);
        };
    };

    /*** Remap Tables initialisieren ***/
    ywd->TracyRemap = _new("ilbm.class",
                           RSA_Name, "remap/tracyrmp.ilbm",
                           TAG_DONE);
    if (ywd->TracyRemap) {
        _get(ywd->TracyRemap, BMA_Bitmap, &tracy_rmp);
    } else {
        _LogMsg("Couldn't load tracy remap table, set %d.\n",set_num);
        return(FALSE);
    };

    ywd->ShadeRemap = _new("ilbm.class",
                           RSA_Name, "remap/shadermp.ilbm",
                           TAG_DONE);
    if (ywd->ShadeRemap) {
        _get(ywd->ShadeRemap, BMA_Bitmap, &shade_rmp);
    } else {
        _LogMsg("Couldn't load shade remap table, set %d.\n",set_num);
        return(FALSE);
    };

    _OVE_SetAttrs(OVET_TracyRemap, tracy_rmp,
                  OVET_ShadeRemap, shade_rmp,
                  TAG_DONE);

    /*** BeeBox-Object ***/
    ywd->BeeBox = _load("rsrc:objects/beebox.base");
    if (ywd->BeeBox) {
        _set(ywd->BeeBox, BSA_Static, TRUE);
    } else {
        _LogMsg("Couldn't load bbox object, set %d.\n",set_num);
        return(FALSE);
    };

    /*** falls nicht Set46, Fonts *NICHT* set-spezifisch ***/
    if ((set_num==46)||(set_num==42)) {
        if (!yw_LoadFontSet(ywd)) return(FALSE);
    };
    
    /*** Mauspointer initialisieren ***/
    yw_InitMouse(ywd);

    /*** that's all ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
BOOL yw_LoadSky(struct ypaworld_data *ywd, UBYTE *name)
/*
**  FUNCTION
**      Lädt das Himmels-Objekt für diesen Level.
**
**  CHANGED
**      27-Jan-96   floh    created
**      04-Jun-97   floh    alle Himmel werden jetzt aus dem Skypool
**                          geladen
*/
{
    UBYTE fn[255];

    strcpy(fn,"data:");
    strcat(fn,name);
    ywd->Heaven = _load(fn);
    if (ywd->Heaven) {
        _set(ywd->Heaven,BSA_Static,TRUE);
        _set(ywd->Heaven,BSA_VisLimit,ywd->HeavenVisLimit);
        _set(ywd->Heaven,BSA_DFadeLength,ywd->HeavenDFadeLen);
    } else {
        _LogMsg("Couldn't create %s\n",fn);
        return(FALSE);
    };

    return(TRUE);
}

/*-----------------------------------------------------------------*/
BOOL yw_LoadTypMap(struct ypaworld_data *ywd, UBYTE *name)
/*
**  FUNCTION
**      Lädt Type-Map und initialisiert dabei:
**          ywd->MapSizeX
**          ywd->MapSizeY
**          ywd->CellArea[n].BactList
**                          .Type
**                          .SType
**                          .SubEnergy
**
**  CHANGED
**      27-Jan-96   floh    created
**      23-May-96   floh    Im LevelDesigner-Mode wird die
**                          Map jetzt nicht disposed, sondern
**                          bleibt während des Spiels erhalten.
**      11-Oct-96   floh    Map bekommt ILBMA_SaveILBM Attr auf TRUE.
**      14-Oct-96   floh    + TypeMap wird mit ColorMap erzeugt
**      20-Oct-96   floh    + TypeMap wird jetzt auch mit "Normal-Modus"
**                            erhalten
**      23-Jun-97   floh    + Map-In-LDF-Support (in diesem Fall wurden
**                            Maps schon beim LDF-Parsen initialisiert
**                            und werden somit nicht neu geladen!).
**      23-Mar-98   floh    + kein Support mehr für externe Maps
*/
{
    if (ywd->TypeMap) {

        struct VFMBitmap *bmp;
        UBYTE *d;
        ULONG i,size;

        _get(ywd->TypeMap, BMA_Bitmap, &bmp);
        d = bmp->Data;

        ywd->MapSizeX   = bmp->Width;
        ywd->MapSizeY   = bmp->Height;
        ywd->WorldSizeX = ywd->MapSizeX * SECTOR_SIZE;
        ywd->WorldSizeY = ywd->MapSizeY * SECTOR_SIZE;
        size            = ywd->MapSizeX * ywd->MapSizeY;

        /*** CellArea initialisieren ***/
        memset(ywd->CellArea,0,size*sizeof(struct Cell));

        for (i=0; i<size; i++) {

            struct Cell *sec = &(ywd->CellArea[i]);
            UBYTE type = d[i];
            struct SectorDesc *sd = &(ywd->Sectors[type]);
            ULONG k,l,lim;

            _NewList((struct List *) &(sec->BactList));

            sec->Type         = type;
            sec->SType        = sd->SecType;
            sec->EnergyFactor = 0;

            /*** Anfangs-Energy initialisieren ***/
            if (sd->SecType == SECTYPE_COMPACT) lim = 1;
            else                                lim = 3;
            for (k=0; k<lim; k++) {
                for (l=0; l<lim; l++) {
                    sec->SubEnergy[k][l] = sd->SSD[k][l]->init_energy;
                };
            };
        };
    } else return(FALSE);
    return(TRUE);
}

/*-----------------------------------------------------------------*/
BOOL yw_LoadOwnMap(struct ypaworld_data *ywd, UBYTE *name)
/*
**  FUNCTION
**      Lädt Owner-Map und initialisiert dabei:
**          ywd->CellArea[n].Owner
**
**      Randsektoren werden für den Floodfiller immer
**      auf (Owner == 0) initialisiert.
**
**  CHANGED
**      27-Jan-96   floh    created
**      28-Jan-96   floh    yw_CheckSector() nach hier verlagert,
**                          weil die Routine evtl. den Owner wieder
**                          auf NULL setzen darf (bei leeren Sektoren)
**      29-Jan-96   floh    der Empty-Check wurde in die Building-Schleife
**                          verlagert, damit es keine Probleme gibt
**                          mit leeren Sektoren auf die gebaut werden
**                          soll.
**      23-May-96   floh    Im LevelDesigner-Mode wird die
**                          Map jetzt nicht disposed, sondern
**                          bleibt während des Spiels erhalten.
**      11-Oct-96   floh    + Map bekommt ILBMA_SaveILBM Attr auf TRUE.
**      14-Oct-96   floh    + Map wird jetzt mit ColorMap erzeugt
**      20-Oct-96   floh    + Owner-Map wird jetzt auch im Normalmodus
**                            erhalten
**      02-Nov-96   floh    + initialisiert SectorCount[] Array
**      23-Jun-97   floh    + Map-In-LDF-Support (in diesem Fall wurden
**                            Maps schon beim LDF-Parsen initialisiert
**                            und werden somit nicht neu geladen!).
**      23-Mar-98   floh    + kein Support mehr für externe Maps
*/
{
    /*** Sector-Counter auf 0 ***/
    memset(&(ywd->SectorCount),0,sizeof(ywd->SectorCount));
    if (ywd->OwnerMap) {

        struct VFMBitmap *bmp;
        UBYTE *d;
        ULONG x,y,i;

        _get(ywd->OwnerMap, BMA_Bitmap, &bmp);
        d = bmp->Data;

        /*** richtige Größe? ***/
        if ((ywd->MapSizeX != bmp->Width) || (ywd->MapSizeY != bmp->Height)) {
            _LogMsg("Mapsize mismatch %s: is [%d,%d], should be [%d,%d].\n",
                    name, bmp->Width, bmp->Height, 
                    ywd->MapSizeX, ywd->MapSizeY);
            _dispose(ywd->OwnerMap);
            ywd->OwnerMap = NULL;
            return(FALSE);
        };

        for (y=0,i=0; y<ywd->MapSizeY; y++) {
            for (x=0; x<ywd->MapSizeX; x++,i++) {
                struct Cell *sec = &(ywd->CellArea[i]);
                if ((x==0)||(y==0)||(x==(ywd->MapSizeX-1))||(y==(ywd->MapSizeY-1)))
                {
                    sec->Owner = 0;
                } else {
                    sec->Owner = d[i];
                };
                /*** Sector-Counter updaten ***/
                ywd->SectorCount[sec->Owner]++;
            };
        };
    } else return(FALSE);
    return(TRUE);
}

/*-----------------------------------------------------------------*/
BOOL yw_LoadHgtMap(struct ypaworld_data *ywd, UBYTE *name)
/*
**  FUNCTION
**      Lädt Höhen-Map und initialisiert dabei:
**          ywd->CellArea[n].Owner
**
**  CHANGED
**      27-Jan-96   floh    created
**      23-May-96   floh    Im LevelDesigner-Mode wird die
**                          Map jetzt nicht disposed, sondern
**                          bleibt während des Spiels erhalten.
**      11-Oct-96   floh    + Map bekommt ILBMA_SaveILBM Attr auf TRUE.
**      14-Oct-96   floh    + Map wird jetzt mit ColorMap erzeugt
**      20-Oct-96   floh    + HgtMap wird jetzt auch im Normal-Modus
**                            erhalten
**      23-Jun-97   floh    + Map-In-LDF-Support (in diesem Fall wurden
**                            Maps schon beim LDF-Parsen initialisiert
**                            und werden somit nicht neu geladen!).
**      23-Mar-98   floh    + kein Support mehr für externe Maps
*/
{
    if (ywd->HeightMap) {

        struct VFMBitmap *bmp;
        UBYTE *d;
        ULONG i,j,size;

        _get(ywd->HeightMap, BMA_Bitmap, &bmp);
        d = bmp->Data;

        /*** richtige Größe? ***/
        if ((ywd->MapSizeX != bmp->Width) || (ywd->MapSizeY != bmp->Height)) {
            _LogMsg("Mapsize mismatch %s: is [%d,%d], should be [%d,%d].\n",
                    name, bmp->Width, bmp->Height, 
                    ywd->MapSizeX, ywd->MapSizeY);
            _dispose(ywd->HeightMap);
            ywd->HeightMap = NULL;
            return(FALSE);
        };

        /*** Sektor-Höhen eintragen ***/
        size = ywd->MapSizeX * ywd->MapSizeY;
        for (i=0; i<size; i++) {
            struct Cell *sec = &(ywd->CellArea[i]);
            sec->Height = (FLOAT) d[i] * SECTOR_HEIGHTSCALE;
        };

        /*** Durchschnitts-Höhen an Kreuzungs-Punkten bestimmen ***/
        for (i=1; i<(ywd->MapSizeY); i++) {
            for (j=1; j<(ywd->MapSizeX); j++) {

                LONG off1 = i*ywd->MapSizeX + j;
                LONG off2 = (i-1)*ywd->MapSizeX + (j-1);
                LONG off3 = (i-1)*ywd->MapSizeX + j;
                LONG off4 = i*ywd->MapSizeX + (j-1);

                struct Cell *s1 = &(ywd->CellArea[off1]);
                struct Cell *s2 = &(ywd->CellArea[off2]);
                struct Cell *s3 = &(ywd->CellArea[off3]);
                struct Cell *s4 = &(ywd->CellArea[off4]);

                s1->AvrgHeight = (s1->Height +
                                  s2->Height +
                                  s3->Height +
                                  s4->Height)/4.0;
            };
        };
    } else return(FALSE);
    return(TRUE);
}

/*-----------------------------------------------------------------*/
BOOL yw_LoadBlgMap(Object *world, struct ypaworld_data *ywd, UBYTE *name)
/*
**  FUNCTION
**      Lädt Building-Map und baut für jedes Pixel welches != 0
**      ist den entsprechenden Building-Prototype im Immediate-
**      Modus.
**
**  CHANGED
**      27-Jan-96   floh    created
**      29-Jan-96   floh    wendet YWM_CREATEBUILDING im
**                          Immediate-Modus an
**      23-May-96   floh    Im LevelDesigner-Mode wird die
**                          Map jetzt nicht disposed, sondern
**                          bleibt während des Spiels erhalten.
**      11-Oct-96   floh    + Map bekommt ILBMA_SaveILBM Attr auf TRUE.
**      14-Oct-96   floh    + Map wird jetzt mit ColorMap erzeugt
**      20-Oct-96   floh    + BlgMap wird jetzt auch im Normalmodus
**                            erhalten
**      23-Jun-97   floh    + Map-In-LDF-Support (in diesem Fall wurden
**                            Maps schon beim LDF-Parsen initialisiert
**                            und werden somit nicht neu geladen!).
**      23-Mar-98   floh    + kein Support mehr für externe Maps
*/
{
    if (ywd->BuildMap) {

        struct VFMBitmap *bmp;
        UBYTE *d;
        ULONG x,y,i;

        _get(ywd->BuildMap, BMA_Bitmap, &bmp);
        d = bmp->Data;

        /*** richtige Größe? ***/
        if ((ywd->MapSizeX != bmp->Width) || (ywd->MapSizeY != bmp->Height)) {
            _LogMsg("Mapsize mismatch %s: is [%d,%d], should be [%d,%d].\n",
                    name, bmp->Width, bmp->Height, 
                    ywd->MapSizeX, ywd->MapSizeY);
            _dispose(ywd->BuildMap);
            ywd->BuildMap = NULL;
            return(FALSE);
        };

        for (y=0,i=0; y<ywd->MapSizeY; y++) {
            for (x=0; x<ywd->MapSizeX; x++,i++) {

                struct Cell *sec = &(ywd->CellArea[i]);

                if ((d[i] != 0) && (sec->Owner != 0)) {

                    struct createbuilding_msg cv_msg;

                    cv_msg.job_id    = sec->Owner;
                    cv_msg.owner     = sec->Owner;
                    cv_msg.bp        = d[i];
                    cv_msg.immediate = TRUE;
                    cv_msg.sec_x     = x;
                    cv_msg.sec_y     = y;
                    cv_msg.flags     = 0;
                    _methoda(world, YWM_CREATEBUILDING, &cv_msg);
                };
            };
        };
    } else return(FALSE);
    return(TRUE);
}

/*-----------------------------------------------------------------*/
BOOL yw_PlaceRobos(Object *world, struct ypaworld_data *ywd,
                   ULONG num_robos, struct NLRoboDesc *robos)
/*
**  FUNCTION
**      Erzeugt neue Robos und plaziert diese in der Welt.
**      Die Informationen zur Erschaffung der Robos
**      stehen im übergebenen <struct NCRoboDesc[]>.
**
**  INPUTS
**      world   - Ptr auf Welt-Object
**      ywd     - Ptr auf dessen LID
**      num_robos   - Anzahl Robos in Welt
**      robos       - NLRoboDesc-Array mit <num_robos> Einträgen
**
**  RESULTS
**      TRUE/FALSE
**
**  CHANGED
**      02-Feb-96   floh    created
**      04-Feb-96   floh    + smartere Plazierung
**      12-Feb-96   floh    + Energie-Budgets
**      03-May-96   floh    umgeschrieben für NLRoboDesc-Struktur
**      20-Jun-96   floh    + statt Sektor-Koordinaten wird jetzt
**                            eine "exakte" 3D-Position verlangt
**      08-Jul-96   floh    + if (ywd->LevelMode == LEVELMODE_SEQPLAYER)
**                            werden KEINE Robos erzeugt!
**      20-Jul-96   floh    + YourLastSeconds-Patch auf Robo
**                            rausgenommen
**      03-Aug-96   floh    + revised & updated (AF's neues
**                            Batterie-Handling
**      04-Aug-96   floh    + der 1.(User-) Robo bekommt seine
**                            Batterie-Werte geviertelt
**      21-Sep-96   floh    + im Design-Modus werden keine
**                            Feindrobos erzeugt
**      08-Sep-97   floh    + füllt <ywd->LevelMask> und <ywd->UserMask>
**                            aus
**      22-Apr-98   floh    + RoboReloadConst
*/
{
    if (LEVELMODE_SEQPLAYER != ywd->Level->Mode) {

        ULONG i;

        ywd->Level->OwnerMask = 0;
        ywd->Level->UserMask  = (1<<1);
        for (i=0; i<num_robos; i++) {

            struct createvehicle_msg cv;
            struct intersect_msg ins;
            Object *act_robo;

            #ifdef YPA_DESIGNMODE
            if (robos[i].owner != 1) continue;
            #endif

            /*** genaue Start-Position (Höhe über Boden) ermitteln ***/
            ins.pnt.x = robos[i].pos.x;
            ins.pnt.y = -30000.0;
            ins.pnt.z = robos[i].pos.z;
            ins.vec.x = 0.0;
            ins.vec.y = 50000.0;
            ins.vec.z = 0.0;
            ins.flags = 0;
            cv.vp = robos[i].vhcl_proto;
            cv.x  = robos[i].pos.x;
            cv.y  = robos[i].pos.y;
            cv.z  = robos[i].pos.z;

            _methoda(world, YWM_INTERSECT, &ins);

            /*** falls Intersection, Y-Start-Pos anpassen ***/
            if (ins.insect) cv.y = ins.ipnt.y + robos[i].pos.y;

            /*** Robo erzeugen ***/
            act_robo = (Object *) _methoda(world, YWM_CREATEVEHICLE, &cv);
            if (act_robo) {

                struct Bacterium *bact;
                LONG energy, reload_const = 0;

                _methoda(world, YWM_ADDCOMMANDER, act_robo);

                _get(act_robo, YBA_Bacterium, &bact);
                if (i==0) {
                    /*** der User-Robo ***/
                    energy = robos[i].energy>>2;
                    if (energy < ywd->MaxRoboEnergy) {
                        energy       = ywd->MaxRoboEnergy;
                        reload_const = ywd->MaxReloadConst;
                    };
                } else {
                    /*** ein autonomer Robo ***/
                    energy = robos[i].energy;
                };                    
                
                /*** falls ReloadConst noch nicht durch MaxReloadConst zugewiesen ***/
                bact->Owner   = robos[i].owner;
                bact->Energy  = energy;
                bact->Maximum = energy;
                if (reload_const == 0) {    
                    if (robos[i].robo_reload_const) reload_const = robos[i].robo_reload_const;
                    else                            reload_const = bact->Maximum;
                };
                bact->RoboReloadConst = reload_const;
                _set(act_robo, YBA_BactCollision, TRUE);
                _set(act_robo, YRA_FillModus, YRF_Fill_All);
                _set(act_robo, YRA_BattVehicle, energy);
                _set(act_robo, YRA_BattBeam, energy);
                
                /*** Masken in ywd->Level ausfüllen ***/
                ywd->Level->OwnerMask |= (1<<robos[i].owner);

                /*** Energie-Budget einstellen ***/
                _set(act_robo, YRA_EPConquer, robos[i].conquer);
                _set(act_robo, YRA_EPDefense, robos[i].defense);
                _set(act_robo, YRA_EPRadar,   robos[i].radar);
                _set(act_robo, YRA_EPPower,   robos[i].power);

                /*** neu: 8100 000C ***/
                _set(act_robo, YRA_EPSafety,      robos[i].safety);
                _set(act_robo, YRA_EPChangePlace, robos[i].place);
                _set(act_robo, YRA_EPRobo,        robos[i].robo);
                _set(act_robo, YRA_EPReconnoitre, robos[i].recon);

                /*** neu: Blickrichtung ***/
                _set(act_robo, YRA_ViewAngle, robos[i].viewangle);

                /*** Neu: Delays koenen einen Anfangswert haben ***/
                _set(act_robo, YRA_SafDelay, robos[i].saf_delay);
                _set(act_robo, YRA_PowDelay, robos[i].pow_delay);
                _set(act_robo, YRA_CplDelay, robos[i].cpl_delay);
                _set(act_robo, YRA_RadDelay, robos[i].rad_delay);
                _set(act_robo, YRA_DefDelay, robos[i].def_delay);
                _set(act_robo, YRA_ConDelay, robos[i].con_delay);
                _set(act_robo, YRA_RecDelay, robos[i].rec_delay);
                _set(act_robo, YRA_RobDelay, robos[i].rob_delay);

                /*** 1.Robo ist immer Viewer ***/
                if (0 == i) {
                    _set(act_robo, YBA_Viewer, TRUE);
                    _set(act_robo, YBA_UserInput, TRUE);
                };
            };
        };
    };

    return(TRUE);
}

/*-----------------------------------------------------------------*/
void yw_LoadSISamples(struct SoundInit *si)
/*
**  FUNCTION
**      Lädt die Samples der angegebenen Sample-Definition.
**      "Extended Sample Definition" overrides die
**      althergebrachte "Einzel-Sample-Definition".
**
**  CHANGED
**      22-Dec-96   floh    created
**      19-Aug-97   floh    + Anpassung an Load-On-Demand (Test, ob
**                            Sample-Objekte schon geladen wurden,
**                            wenn ja, Abbruch)
*/
{
    UBYTE old_path[255];

    /*** wenn bereits geladen, wieder raus ***/
    if ((si->ext_smp_obj[0]) || (si->smp_object)) {
        return;
    };

    /*** Resource-Path temporär umbiegen ***/
    strcpy(old_path, _GetAssign("rsrc"));
    _SetAssign("rsrc", "data:");
    if (si->ext_smp.count > 0) {

        ULONG i;

        /*** Extended Sample Definition existiert ***/
        for (i=0; i<si->ext_smp.count; i++) {
            si->ext_smp_obj[i] = _new("wav.class",
                                 RSA_Name, si->ext_smp_name[i],
                                 TAG_DONE);
            if (si->ext_smp_obj[i]) {

                struct VFMSample *smp;
                struct SampleFragment *sf = &(si->ext_smp.frag[i]);

                /*** Pointer auf VFMSample-Struktur ***/
                _get(si->ext_smp_obj[i],SMPA_Sample,&smp);
                sf->sample = smp;

                /*** real_offset und real_len berechnen ***/
                sf->real_offset = (sf->offset*smp->SamplesPerSec)/11000;
                sf->real_len    = (sf->len*smp->SamplesPerSec)/11000;
                if (sf->real_offset>smp->Length) sf->real_offset=smp->Length;
                if (sf->real_len == 0) sf->real_len = smp->Length;
                if ((sf->real_offset + sf->real_len) > smp->Length) {
                    sf->real_len = smp->Length - sf->real_offset;
                };
            } else {
                _LogMsg("Warning: Could not load sample %s.\n", si->ext_smp_name[i]);
            };
        };

    } else {

        /*** keine ESD, vielleicht eine herkömmliche? ***/
        if (si->smp_name[0]) {
            si->smp_object = _new("wav.class",
                             RSA_Name, si->smp_name,
                             TAG_DONE);
            if (NULL == si->smp_object) {
                _LogMsg("Warning: Could not load sample %s.\n", si->smp_name);
            };
        };
    };

    /*** Resource-Pfad wieder zurück ***/
    _SetAssign("rsrc", old_path);

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yw_KillSISamples(struct SoundInit *si)
/*
**  FUNCTION
**      Killt alle Sample-Objects in der angegebenen Sample-
**      Definition.
**
**  CHANGED
**      22-Dec-96   floh    created
*/
{
    ULONG i;

    /*** Extended-Sample-Definition? ***/
    for (i=0; i<si->ext_smp.count; i++) {
        if (si->ext_smp_obj[i]) {
            _dispose(si->ext_smp_obj[i]);
            si->ext_smp_obj[i] = NULL;
        };
    };

    /*** normale Sample-Definition ***/
    if (si->smp_object) {
        _dispose(si->smp_object);
        si->smp_object = NULL;
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
BOOL yw_LoadSoundSamples(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Lädt die SoundSamples, die per Name in den
**      Elementen von VP_Array, WP_Array, BP_Array
**      (und neu: ywd->GUISoundInit) zu finden sind.
**
**      Falls ein Sample nicht geladen werden konnte,
**      wird nur eine Warnung ausgegeben. Im Spiel
**      wird dieses Sample einfach nicht zu hören sein.
**
**  CHANGED
**      23-Apr-96   floh    created
**      28-May-96   floh    + GUI-Sound-Init
**      20-Dec-96   floh    + angepaßt an neues Sample-Handling
**      12-Apr-97   floh    + VP_Array nicht mehr global
**                          + WP_Array nicht mehr global
**                          + BP_Array nicht mehr global
**      18-Jun-97   floh    + beachtet jetzt die Audio an/aus
**                            Voreinsteller
**      19-Aug-97   floh    + alle Samples, die an Vehicles, Weapons,
**                            Buildings hängen, werden jetzt erst
**                            geladen, wenn das erste Objekt dieses
**                            Typs erzeugt wird.
**      25-Oct-97   floh    + GUISoundCarrier ist rausgeflogen, jetzt
**                            alles über ShellSound1/2, da die VehicleSounds
**                            jetzt on-touch geladen werden, ist die
**                            Routine damit leer.
*/
{
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void yw_KillSoundSamples(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Gegenstück zu yw_LoadSoundSamples()
**
**  CHANGED
**      23-Apr-96   floh    created
**      28-May-96   floh    + GUI-Sounds
**      02-Jan-96   floh    + macht jetzt zuerst ein _FlushAudio()
**      12-Apr-97   floh    + VP_Array nicht mehr global
**                          + WP_Array nicht mehr global
**                          + BP_Array nicht mehr global
**      25-Oct-97   floh    + GUISoundCarrier ist rausgeflogen
*/
{
    ULONG i,j;

    /*** interne Audioengine-Puffer leeren ***/
    _FlushAudio();

    for (i=0; i<NUM_VEHICLEPROTOS; i++) {
        struct VehicleProto *vp = &(ywd->VP_Array[i]);
        for (j=0; j<VP_NUM_NOISES; j++) yw_KillSISamples(&(vp->Noise[j]));
    };

    for (i=0; i<NUM_WEAPONPROTOS; i++) {
        struct WeaponProto *wp = &(ywd->WP_Array[i]);
        for (j=0; j<WP_NUM_NOISES; j++) yw_KillSISamples(&(wp->Noise[j]));
    };

    for (i=0; i<NUM_BUILDPROTOS; i++) {
        struct BuildProto *bp = &(ywd->BP_Array[i]);
        for (j=0; j<BP_NUM_NOISES; j++) yw_KillSISamples(&(bp->Noise[j]));
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yw_InitWundersteins(Object *world, struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Scannt das Wunderstein-Array in der Weltklassen-LID
**      (gem[]) und setzt Wunderstein-Markierungen in's CellArea.
**      Dazu wird WType auf WTYPE_Wunderstein gesetzt.
**      Sollte der Sektor ein WTYPE_Kraftwerk sein, wird
**      dieses vorher standesgemäß gekillt!
**
**  CHANGED
**      04-May-96   floh    created
**      05-May-96   floh    An die Stelle des Wundersteins wird das
**                          bei der Wunderstein-Definition angegebene
**                          Building gesetzt. Weil das per YWM_CREATEBUILDING
**                          passiert, müssen Kraftwerke auch nicht mehr
**                          explizit gekillt werden.
**      22-Sep-97   floh    Guckt jetzt nach, ob das Wunderstein-Gebäude
**                          schon steht. Wenn ja, wird nix mehr drüber gebaut.
**                          Das sollte diverse Zuständigkeits-Probleme
**                          und Doppelgemoppel bei LoadGames, etc...
**                          beseitigen.
**      16-Dec-97   floh    initialisiert Touchstone Struktur
*/
{
    ULONG i;

    /*** Touchstone Struktur initialisieren ***/
    ywd->touch_stone.gem        = -1;
    ywd->touch_stone.time_stamp = 0;
    ywd->touch_stone.bp_num     = 0;
    ywd->touch_stone.vp_num     = 0;
    ywd->touch_stone.wp_num     = 0;

    /*** Wundersteine themselves initialisieren ***/
    for (i=0; i<MAXNUM_WUNDERSTEINS; i++) {
        if (ywd->gem[i].active) {

            ULONG sec_x = ywd->gem[i].sec_x;
            ULONG sec_y = ywd->gem[i].sec_y;
            struct Cell *sec = &(ywd->CellArea[sec_y * ywd->MapSizeX + sec_x]);

            /*** Wunderstein-Building hinsetzen, so vorhanden ***/
            if (ywd->gem[i].bproto != 0) {
                /*** gebaut wird nur, wenn das Gebäude nicht schon steht ***/
                if (!((sec->WType == WTYPE_Building) &&
                      (sec->WIndex == ywd->gem[i].bproto)))
                {
                    struct createbuilding_msg cv_msg;
                    cv_msg.job_id    = sec->Owner;
                    cv_msg.owner     = sec->Owner;
                    cv_msg.bp        = ywd->gem[i].bproto;
                    cv_msg.immediate = TRUE;
                    cv_msg.sec_x     = sec_x;
                    cv_msg.sec_y     = sec_y;
                    cv_msg.flags     = 0;
                    _methoda(world, YWM_CREATEBUILDING, &cv_msg);
                };
            };

            /*** und Wunderstein reinpatchen ***/
            sec->WType  = WTYPE_Wunderstein;
            sec->WIndex = i;
        };
    };
}

/*-----------------------------------------------------------------*/
void yw_InitPaletteSlots(struct ypaworld_data *ywd, struct LevelDesc *l)
/*
**  FUNCTION
**      Initialisiert die Paletten-Slots der Gfx-Engine
**      nach den Filenamen in der LevelDesc-Struktur.
**
**  CHANGED
**      14-Aug-96   floh    created
*/
{
    Object *gfx_o;
    ULONG i;

    /*** Handle des Gfx-Objects (display.class oder abgeleitet) ***/
    _OVE_GetAttrs(OVET_Object, &gfx_o, TAG_DONE);
    for (i=0; i<8; i++) {

        /*** aktueller Slot initialisiert? ***/
        if (l->pal_slot[i][0]) {

            Object *pal_o;

            /*** erzeuge Paletten-Bitmap-Object ***/
            pal_o = _new("ilbm.class",
                         RSA_Name, &(l->pal_slot[i][0]),
                         BMA_HasColorMap, TRUE,
                         TAG_DONE);
            if (pal_o) {
                struct disp_setpal_msg dsm;
                dsm.slot  = i;
                dsm.first = 0;
                dsm.num   = 256;
                _get(pal_o,BMA_ColorMap,&(dsm.pal));
                if (i==0) {
                    /*** Slot0 initialisiert Default-Palette ***/
                    _set(gfx_o,BMA_ColorMap,dsm.pal);
                } else {
                    /*** alle anderen Slots nur ausfüllen ***/
                    _methoda(gfx_o, DISPM_SetPalette, &dsm);
                };
                _dispose(pal_o);
            }else{
                _LogMsg("WARNING: slot #%d [%s] init failed!\n",i,&(l->pal_slot[i][0]));
            };
        };
    };
}

/*-----------------------------------------------------------------*/
_dispatcher(void, yw_YWM_KILLLEVEL, void *ignored)
/*
**  FUNCTION
**      Falls ein Level geladen ist, werden alle in
**      YWM_NEWLEVEL erzeugten Sachen gekillt.
**
**  CHANGED
**      27-Jan-96   floh    created
**      23-Apr-96   floh    revised & updated
**      24-Apr-96   floh    + yw_KillGUIModule()
**                          + yw_KillEnergyModule()
**      23-May-96   floh    + falls im DesignModus, werden die
**                            (noch existierenden) Map-Bitmaps
**                            rausgeschrieben, und anschließend
**                            gekillt.
**      30-Aug-96   floh    + schaltet Mauspointer in Cancel-Modus
**      02-Oct-96   floh    + yw_DoLevelStatus() Cleanup
**                          + Bildschirm wird gelöscht
**                          + Audio-MasterVolume wird restauriert
**      20-Oct-96   floh    + da alle Maps erhalten bleiben, müssen
**                            sie auch fachgerecht gekillt werden
**                            (nicht nur im Design-Modus)
**      15-Jan-97   floh    + Cleanup Maus-Pointer
**      05-Apr-97   floh    + AF's Network-Sachen (yw_CleanupNetworkSession)
**      31-May-97   floh    + yw_ShowDiskAccess()
**      06-Aug-97   floh    + CleanupPrototypeArrays ist rausgeflogen,
**                            wegen den Vehicle-Wireframes
**      24-Sep-97   floh    + yw_KillVoiceOverSystem()
**      23-Oct-97   floh    + ShellSound1 und ShellSound2 werden auf
**                            Position 0 zurückgesetzt.
**      09-Apr-98   floh    + DoDebriefing wird initialisiert
**      18-Apr-98   floh    + yw_KillEventCatcher()
**      18-May-98   floh    + Bug: Zugriff auf Robo, der aber gar
**                            existieren musste...
**      28-May-98   floh    + Debriefing wird nur abgespielt, wenn
**                            Level gewonnen, oder Multiplayer
**      01-Jun-98   floh    + falls der Level eine Eventloop hatte, wird
**                            er IMMER auf LEVELSTAT_ABORTED geschaltet
**                            (passiert in yw_DoLevelStatus())
**                                  
*/                           
{
    struct ypaworld_data *ywd = INST_DATA(cl,o);
    UBYTE   user_owner;
    Object *da_pic;

    /*** LevelStatus-spezieller Cleanup (muss als erstes kommen!) ***/
    yw_DoLevelStatus(ywd);

    /*** Wenn User gewonnen hat, Debriefing anschalten ***/ 
    if (ywd->Level->Status == LEVELSTAT_FINISHED) {
        /*** Single-Player-Debriefing aktivieren ***/
        ywd->DoDebriefing = TRUE;
        /*** FirstContact Handling ***/
        if (ywd->FirstContactOwner != 0) ywd->Level->RaceTouched[ywd->FirstContactOwner] = TRUE;
    } else {
        ywd->DoDebriefing = FALSE;
    };
    
    /* -------------------------------------------------------------
    ** Wenn Level korrekt verlassen wurde, ist jedes InGame-Savegame
    ** aelter als der aktuelle Stand. Deshalb merken wir uns das mit
    ** Hilfe eines Flags in Form eines Files
    ** -----------------------------------------------------------*/
    if( (ywd->Level->Status == LEVELSTAT_FINISHED) && ywd->gsr ) {
        
        FILE  *f;
        char   filename[ 300 ];
        struct saveloadsettings_msg sls;
    
        sprintf( filename, "save:%s/sgisold.txt", ywd->gsr->UserName );
        if( f = _FOpen( filename, "w" ))
            _FClose(f);
        
        /*** Abspeichern der Default-settings ***/
        sprintf( filename, "%s/user.txt", ywd->gsr->UserName );
        sls.filename = filename;
        sls.username = ywd->gsr->UserName;
        sls.gsr      = ywd->gsr;
        sls.mask     = DM_SCORE|DM_USER|DM_INPUT|DM_VIDEO|DM_SHELL|DM_SOUND|
                       DM_BUILD|DM_BUDDY;
        sls.flags    = 0;
        _methoda( o, YWM_SAVESETTINGS, &sls );
    
        /*** Abspeichern des letzten Spielers ***/
        if( f = _FOpen( "env:user.def", "w") ) {
    
            strcpy( filename, ywd->gsr->UserName );
            _FWrite( filename, strlen( filename ), 1, f );
            _FClose( f );
            }
        }
        
    if( ywd->playing_network ) {
        /*** letzte daten rausschreiben ***/
        yw_PrintNetworkInfoEnd( ywd->gsr );
        yw_CleanupNetworkSession( ywd );
        ywd->DoDebriefing      = TRUE;
        ywd->WasNetworkSession = TRUE;
    } else {
        ywd->WasNetworkSession = FALSE;
    };

    /*** falls Seq-Recording noch aktiv, dieses abschließen ***/
    if (ywd->out_seq->active) yw_RCEndScene(ywd);
    ywd->seq_digger = FALSE;
    ywd->out_seq->active = FALSE;

    /*** Anzeigen, das was passiert... ***/
    if (da_pic = yw_BeginDiskAccess(ywd)) {
        yw_ShowDiskAccess(ywd,da_pic);
        yw_EndDiskAccess(ywd,da_pic);
    };
    
    /*** EventCatcher killen ***/
    yw_KillEventCatcher(ywd);   

    /*** VoiceOver System aufräumen ***/
    yw_KillVoiceOverSystem(ywd);

    /*** MasterVolume restaurieren ***/
    _AE_SetAttrs(AET_MasterVolume,ywd->MasterVolume,TAG_DONE);

    /*** Game-GUI abschießen ***/
    yw_KillGUIModule(o,ywd);

    /*** Heaven-Object killen, falls eins da ***/
    if (ywd->Heaven) {
        _dispose(ywd->Heaven);
        ywd->Heaven = NULL;
    };

    /*** alle Objects im Deathcache killen ***/
    if (ywd->URBact) user_owner = ywd->URBact->Owner;    // merken bevor er freigegeben wird
    else             user_owner = 0;
    while (ywd->DeathCache.mlh_Head->mln_Succ) {
    
        struct OBNode *robo = (struct OBNode *) ywd->DeathCache.mlh_Head;
        Object *death = ((struct OBNode *)ywd->DeathCache.mlh_Head)->o;
        
        if( (ywd->WasNetworkSession) &&
            (robo->bact->Owner       != user_owner) &&
            (robo->bact->BactClassID == BCLID_YPAROBO) ) {
                    
            /*** Freigabe ohne DIE, weil ein Schatten ***/
            yw_RemoveAllShadows( ywd, robo );
            
            /*** Flags sind gesetzt, nun erfolgt dispose Aufruf ohne DIE ***/
        }
            
        /*** OriginalVehicle klassisch aufraeumen ***/ 
        _dispose(death);
    };

    /*** alle Commanders metzeln ***/
    while (ywd->CmdList.mlh_Head->mln_Succ) {
    
        struct OBNode *robo = (struct OBNode *) ywd->CmdList.mlh_Head;
        Object *cmd = ((struct OBNode *)ywd->CmdList.mlh_Head)->o;

        if( (ywd->WasNetworkSession) &&
            (robo->bact->Owner       != user_owner) &&
            (robo->bact->BactClassID == BCLID_YPAROBO) ) {
                    
            /*** Freigabe ohne DIE, weil ein Schatten ***/
            yw_RemoveAllShadows( ywd, robo );
            
            /*** Flags sind gesetzt, nun erfolgt dispose Aufruf ohne DIE ***/
        }

        _dispose(cmd);
    };
    
    
    /*** durch RemoveAllShadows sind wieder welche im DeathCache ***/
    while (ywd->DeathCache.mlh_Head->mln_Succ) {
    
        Object *death = ((struct OBNode *)ywd->DeathCache.mlh_Head)->o;
        _dispose(death);
    };

    /*** Vehicle, Weapon, Building-Soundsamples killen ***/
    yw_KillSoundSamples(ywd);

    /*** Set killen ***/
    yw_KillSet(ywd);

    /*** Energie-Modul killen ***/
    yw_KillEnergyModule(ywd);

    /*** Map-Bitmaps killen ***/
    if (ywd->TypeMap) {
        _dispose(ywd->TypeMap);
        ywd->TypeMap = NULL;
    };
    if (ywd->OwnerMap) {
        _dispose(ywd->OwnerMap);
        ywd->OwnerMap = NULL;
    };
    if (ywd->BuildMap) {
        _dispose(ywd->BuildMap);
        ywd->BuildMap = NULL;
    };
    if (ywd->HeightMap) {
        _dispose(ywd->HeightMap);
        ywd->HeightMap = NULL;
    };

    /*** Soundcarrier-Positionen zurücksetzen ***/
    if (ywd->gsr) {
        ywd->gsr->ShellSound1.pos.x = 0.0;
        ywd->gsr->ShellSound1.pos.y = 0.0;
        ywd->gsr->ShellSound1.pos.z = 0.0;
        ywd->gsr->ShellSound2.pos.x = 0.0;
        ywd->gsr->ShellSound2.pos.y = 0.0;
        ywd->gsr->ShellSound2.pos.z = 0.0;
    };

    /* -------------------------------------------------------------
    ** Wenn es ein Netzwerkspiel war, startup-script neu laden und
    ** selbiges mit unseren Einstellungen ueberschreiben, weil Netz-
    ** buildings unterschiedlich sind.
    ** Das ueberschreiben passiert in ypa.c, weil dort eh der ganze 
    ** Kram gemacht wird.
    ** -----------------------------------------------------------*/
    if( ywd->WasNetworkSession ) {
    
        struct saveloadsettings_msg sls;
        char   fn[ 300 ];

        /*** neu initialisieren ***/
        if (ywd->VP_Array && ywd->WP_Array && ywd->BP_Array && ywd->EXT_Array) {
            /*** alles klaa, Startup-Script parsen ***/
            if (!yw_ParseProtoScript(ywd, "data:scripts/startup.scr")) {
                return;
            };
        };
    };

    /*** das war's ***/
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, yw_YWM_NEWLEVEL, struct newlevel_msg *msg)
/*
**  CHANGED
**      08-Jul-96   floh    ruft nur noch YWM_CREATELEVEL auf
*/
{
    struct createlevel_msg clm;
    clm.level_num  = msg->pos_x;
    clm.level_mode = LEVELMODE_NORMAL;
    return(_methoda(o, YWM_CREATELEVEL, &clm));
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, yw_YWM_ADVANCEDCREATELEVEL, struct createlevel_msg *msg)
/*
**  FUNCTION
**      Entspricht YWM_CREATELEVEL, kapselt aber den Test
**      auf "Final-Save-Game"-Test, und legt nach der Initialisierung
**      gleichzeitig ein Savegame an...
**
**  CHANGED
**      01-Aug-97   floh    created
**      06-Aug-97   floh    + Finales Savegame wird nur geladen,
**                            wenn auch der Levelstatus finished ist
**      01-Sep-97   floh    + von Type- und Owner-Map wird jetzt eine
**                            Backup-Kopie gemacht, die fürs Mission-
**                            Briefing verwendet wird.
**      08-Oct-97   floh    + ExistFinalSaveGame() mit veränderten Args
**      09-Jun-98   floh    + nach Finished-Savegames wird die Robo-Energie
**                            auf MaxEnergie gepatcht... 
*/
{
    struct ypaworld_data *ywd = INST_DATA(cl,o);
    char name[128];
    struct saveloadgame_msg slg;
    ULONG retval = FALSE;
    
    /* ---------------------------------------------------------------
    ** Test, ob ein finales Savegame existiert. Denn dann sind wir
    ** in einem bereits gespielten level und laden das zugehörige File
    ** -------------------------------------------------------------*/
    if ((LNSTAT_FINISHED == ywd->LevelNet->Levels[msg->level_num].status) &&
        (yw_ExistFinalSaveGame(msg->level_num, ywd->gsr->UserName)))
    {
        sprintf(name,"save:%s/%d.fin\0",ywd->gsr->UserName,msg->level_num);
        slg.name = name;
        slg.gsr  = ywd->gsr;
        retval = _methoda(o,YWM_LOADGAME,&slg);
        if (!retval) _LogMsg("Warning: in YWM_ADVANCEDCREATELEVEL: YWM_LOADGAME of %s failed!\n",name);
        
        /*** User-Robo-Energie auf MaxEnergie patchen ***/
        ywd->URBact->Energy = ywd->URBact->Maximum;
        
        /*** falls ein Tutorial-Level, die Eventloop abschalten ***/
        if (ywd->EventCatcher) ywd->EventCatcher->event_loop_id = 0;    
    
    } else {
        retval = _methoda(o,YWM_CREATELEVEL,msg);
        if (!retval) _LogMsg("Warning: in YWM_ADVANCEDCREATELEVEL: YWM_CREATELEVEL %d failed!\n",msg->level_num);
    };
    if (retval) {
        /*** Restart-File anlegen ***/
        slg.gsr  = ywd->gsr;
        slg.name = name;
        sprintf(name,"save:%s/%d.rst\0",ywd->gsr->UserName,
                                        ywd->Level->Num);
        if(!_methoda(o,YWM_SAVEGAME,&slg)) {
            _LogMsg("Warning: could not create restart file for level %d, user %s.\n",
                     ywd->Level->Num,ywd->gsr->UserName);
        };
    };

    /*** Backup von Owner und TypeMap ***/
    if (ywd->TypeMapBU) {
        _dispose(ywd->TypeMapBU);
        ywd->TypeMapBU = NULL;
    };
    if (ywd->OwnerMapBU) {
        _dispose(ywd->OwnerMapBU);
        ywd->OwnerMapBU = NULL;
    };
    if (ywd->TypeMap) {
        ywd->TypeMapBU = yw_CopyBmpObject(ywd->TypeMap,"copyof_typemap");
    };
    if (ywd->OwnerMap) {
        ywd->OwnerMapBU = yw_CopyBmpObject(ywd->OwnerMap,"copyof_ownermap");
    };

    return(retval);
}

/*-----------------------------------------------------------------*/
BOOL yw_CommonLevelInit(struct ypaworld_data *ywd,
                        struct LevelDesc *ld,
                        ULONG level_num,
                        ULONG level_mode)
/*
**  FUNCTION
**      Der Teil bei der Level-Initialisierung, der
**      bei allen Spielarten der Level-Initialisierung
**      (normal, LoadGame, Network-Init) einigermaßen
**      gleich ist, so daß nicht so viele Änderungen
**      hin- und hergeupdated werden müssen.
**
**      Folgende Sachen werden gemacht:
**          - yw_ShowDiskAccess()
**          - yw_InitProfiler()
**          - yw_InitHistory()
**          - Initialisierung von Variablen
**          - yw_ParseLDF()
**          - yw_InitEnergyModule()
**          - yw_ResetBuildJobModule()
**          - yw_InitPaletteSlots()
**          - yw_LoadSet()
**          - yw_LoadSky()
**          - Der SysPath_Resources wird auf "levels:"
**            umgebogen.
**
**
**      Der Rest muß weiterhin außerhalb erledigt
**      werden.
**
**      "ld" wurde bei return(TRUE) korrekt von
**      yw_ParseLDF() ausgefüllt.
**
**  CHANGED
**      26-Aug-97   floh    created
**      15-Sep-97   floh    Initialisiert jetzt diverse Pointer mit NULL,
**                          die noch vom letzten Mal übriggeblieben
**                          sein könnten.
**      24-Sep-97   floh    + yw_InitVoiceOverSystem() Aufruf
**      26-Sep-97   floh    + UserInRoboTimeStamp wird zurückgesetzt
**      29-Sep-97   floh    + EnemySectorTimeStamp wird zurückgesetzt
**                          + mehr Pointer werden mit NULL aufgeräumt
**      07-Oct-97   floh    + <ywd->WayPointMode> wird initialisiert
**      28-Oct-97   floh    + GamePaused Initialisierung
**      29-Oct-97   floh    + Joystick-Blanker wird angeschaltet
**      10-Feb-98   floh    + Superitem-Initialisierung
**      01-Mar-98   floh    + GateFullMsgTimeStamp Initialisierung
**      05-Mar-98   floh    + ywd->Url wird initialisiert
**      08-Apr-98   floh    + Softmouse und Texturefiltering-Flags
**                            wurden bei Displaymode-Umschalten nicht
**                            neu gesetzt
**      18-Apr-98   floh    + yw_InitEventCatcher()
**      19-Apr-98   floh    + yw_SetEventLoop()
**      30-Apr-98   floh    + PowerAttackTimeStamp initialisiert
**      09-May-98   floh    + setzt jetzt beim Mode-Umschalten auch 
**                            den Font neu
**      16-May-98   floh    + nucleus.ini Auswertung
**      22-May-98   floh    + ywd->FireDown Initialisierung
**      27-May-98   floh    + Ingame-Stats werden zurueckgesetzt.
**      29-May-98   floh    + Texturefiltering-Flag ist wieder raus...
**                          + UserRoboDied wird initialisiert
**      30-May-98   floh    + Ooops, IngameStats wurden nicht korrekt
**                            zurueckgesetzt...
**      03-Jun-98   floh    + setzt jetzt ywd->netplayerstatus
**                            zurueck
**      06-Jun-98   floh    + VhclSectorRatio initialisiert
*/
{
    BOOL retval = FALSE;
    ULONG res;
    UBYTE old_path[256];
    Object *da_pic;
    UBYTE *tod;
    LONG tod_num;

    /*** allgemeine Initialisierung ***/
    memset(ld,0,sizeof(struct LevelDesc));
    memset(&(ywd->IngameStats),0,sizeof(ywd->IngameStats));
    ywd->TimeStamp            = 0;
    ywd->UserInRoboTimeStamp  = 0;
    ywd->EnemySectorTimeStamp = 0;
    ywd->GateFullMsgTimeStamp = 0;
    ywd->PowerAttackTimeStamp = 0;
    ywd->FrameCount           = 0;
    ywd->Level->Num           = level_num;
    ywd->Level->Mode          = level_mode;
    ywd->Level->Status        = LEVELSTAT_PLAYING;
    ywd->Level->NumGates      = 0;
    ywd->Level->NumItems      = 0;
    ywd->Level->OwnerMask     = 0;
    ywd->Level->UserMask      = 0;
    ywd->SelSector            = NULL;
    ywd->SelBact              = NULL;
    ywd->ClickControlBact     = NULL;
    ywd->Viewer               = NULL;
    ywd->UserRobo             = NULL;
    ywd->UserVehicle          = NULL;
    ywd->URBact               = NULL;
    ywd->UVBact               = NULL;
    ywd->URSlaves             = NULL;
    ywd->WayPointMode         = FALSE;
    ywd->GamePaused           = FALSE;
    ywd->GamePausedTimeStamp  = 0;
    ywd->JoyIgnoreX           = TRUE;
    ywd->JoyIgnoreY           = TRUE;
    ywd->JoyIgnoreZ           = TRUE;
    ywd->Url                  = NULL;
    ywd->LastOccupiedID       = 0;
    ywd->FireDownStatus       = FALSE;
    ywd->FireDown             = FALSE;
    ywd->UserRoboDied         = FALSE;
    ywd->VehicleSectorRatio   = 0.0;

    memset(&(ywd->Level->Gate),0,sizeof(ywd->Level->Gate));
    memset(&(ywd->Level->Item),0,sizeof(ywd->Level->Item));
    memset(&(ywd->gem),0,sizeof(ywd->gem));
    memset(&(ywd->netplayerstatus),0,sizeof(ywd->netplayerstatus));
    memset(&(ywd->VehicleCount),0,sizeof(ywd->VehicleCount));
    
    /*** Statistix rücksetzen ***/
    ywd->MaxSquads   = 0;
    ywd->MaxVehicles = 0;
    ywd->MaxFlaks    = 0;
    ywd->MaxWeapons  = 0;
    ywd->MaxRobos    = 0;

    /*** diverse Inits. ***/
    if (!ywd->OneDisplayRes && (ywd->GameRes != ywd->ShellRes)) {
        /*** Display-Modus auf Spiel-Auflösung schalten ***/
        ULONG do_soft_mouse,do_txt_filtering;
        ULONG mode_info,xres,yres;
        _IE_SetAttrs(IET_ModeInfo, NULL, TAG_DONE);
        _OVE_SetAttrs(OVET_ModeInfo, ywd->GameRes, TAG_DONE);
        _OVE_GetAttrs(OVET_ModeInfo, &mode_info, TAG_DONE);
        _IE_SetAttrs(IET_ModeInfo, mode_info, TAG_DONE);
        _OVE_GetAttrs(OVET_XRes, &xres,
                      OVET_YRes, &yres,
                      OVET_Object, &(ywd->GfxObject),
                      TAG_DONE);
        ywd->DspXRes = xres;
        ywd->DspYRes = yres;
        do_soft_mouse    = ywd->Prefs.Flags & YPA_PREFS_SOFTMOUSE;
        do_txt_filtering = ywd->Prefs.Flags & YPA_PREFS_FILTERING;
        _set(ywd->GfxObject,WINDDA_CursorMode,(do_soft_mouse ? WINDD_CURSORMODE_SOFT:WINDD_CURSORMODE_HW));
        if (ywd->DspXRes < 512) {
            dbcs_SetFont(ypa_GetStr(ywd->LocHandle,STR_FONTDEFINE_LRES,"Arial,8,400,0"));
        } else {
            dbcs_SetFont(ypa_GetStr(ywd->LocHandle,STR_FONTDEFINE,"MS Sans Serif,12,400,0"));
        };
    };
    
    /*** Fontset laden ***/
    if (da_pic = yw_BeginDiskAccess(ywd)) {
        yw_ShowDiskAccess(ywd,da_pic);
    };
    
    strcpy(old_path, _GetAssign("rsrc"));
    _SetAssign("rsrc","data:fonts");
    res = yw_LoadFontSet(ywd);
    _SetAssign("rsrc",old_path);
    if (!res) return(FALSE);
    
    /*** Tip Of The Day Handling ***/
    tod_num = yw_GetIntEnv(ywd,"tod.def");    
    tod = ypa_GetStr(ywd->LocHandle, STR_TIPOFDAY_FIRST + tod_num, " ");
    tod_num++;
    if ((STR_TIPOFDAY_FIRST + tod_num) > STR_TIPOFDAY_LAST) tod_num = 0;
    yw_PutIntEnv(ywd,"tod.def",tod_num);
    if (da_pic) {
        yw_ShowTipOfTheDayDiskAccess(ywd, da_pic, tod);
        yw_EndDiskAccess(ywd,da_pic);
    };        
        
    yw_InitProfiler(ywd);
    yw_InitHistory(ywd);
    _AE_GetAttrs(AET_MasterVolume,&(ywd->MasterVolume),TAG_DONE);
    yw_InitVoiceOverSystem(ywd);
    yw_InitEventCatcher(ywd);
    
    /*** Level-Description-File parsen ***/
    _SetAssign("rsrc","data:");
    if (yw_ParseLDF(ywd,ld,ywd->LevelNet->Levels[ywd->Level->Num].ldf)) {

        /*** fehlt auch nichts??? ***/
        if ((ld->flags & LDESCF_ALL_OK) == LDESCF_ALL_OK) {

            UBYTE set_name[32];
            
            /*** EventLoop initialisieren ***/
            yw_SetEventLoop(ywd,ld->event_loop);            

            /*** Energie-Modul initialisieren ***/
            if (!yw_InitEnergyModule(ywd)) return(FALSE);

            /*** BuildJobs initialisieren ***/
            yw_ResetBuildJobModule(ywd);

            /*** Set-Directory als Resource-Path einstellen ***/
            sprintf(set_name,"data:set%d",ld->set_num);
            _SetAssign("rsrc", set_name);

            /*** Colormap-Slots initialisieren ***/
            yw_InitPaletteSlots(ywd,ld);
            if (yw_LoadSet(ywd,ld->set_num)) {
                if (yw_LoadSky(ywd,ld->sky_name)) {
                    retval = TRUE;
                };
            };
        };
    };
    return(retval);
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, yw_YWM_CREATELEVEL, struct createlevel_msg *msg)
/*
**  CHANGED
**      26-Jan-96   floh    created
**      29-Jan-96   floh    resetted jetzt auch JobArray
**      04-Feb-96   floh    yw_PlaceRobos() und yw_LoadBlgMap()
**                          ausgetauscht, weil YWM_CREATEBUILDING
**                          jetzt auf initialisierte Robos angewiesen
**                          ist (wegen den stationären Bakterien).
**      11-Feb-96   floh    + YWA_LevelFinished -> TRUE
**      19-Mar-96   floh    Bugfix: Routine kam immer mit TRUE zurück
**      23-Apr-96   floh    - Macht jetzt ***kein*** YWM_KILLLEVEL mehr!
**                            Das muß man jetzt per Hand von außen machen!
**                          + Lädt SoundSamples ins VP_Array, WP_Array
**                            und BP_Array
**                          + zusätzliche Keyword <vehicles>, <weapons>,
**                            <buildings> zur Angabe eines Initialisierungs-
**                            Scripts für VP_Array, WP_Array, BP_Array je
**                            Level
**                          + führt VP_Array-Script, WP_Array-Script,
**                            BP_Array-Script aus, wenn vorhanden.
**      24-Apr-96   floh    + yw_InitGUIModule()
**                          + yw_InitEnergyModule()
**      03-May-96   floh    + Benutzt jetzt yw_ParseLDF(), um das
**                            Level-Description-File auszuwerten.
**      10-May-96   floh    + ywd->TimeStamp auf 0
**      17-May-96   floh    + CheckSector akzeptiert jetzt zusätzlich
**                            die X/Y-Koords des Sektors.
**      21-Jun-96   floh    + initialisiert <WarpFader> auf 0.05 (um den
**                            Warp-Effekt beim Rendering zu initialisieren
**      24-Jun-96   floh    + gate_status Initialisierung
**      06-Jul-96   floh    + Nummer des Levels wird nach ywd->LevelNum
**                            geschrieben
**      08-Jul-96   floh    + umbenannt (und umgeschrieben) nach
**                            YWM_CREATELEVEL, mit "modernerer"
**                            Msg.
**      07-Aug-96   floh    + yw_CreateBuddies() (mitgenommene Vehikel)
**      14-Aug-96   floh    + Farbpaletten-Initialisierung ausgelagert
**                            (wegen den Colormap-Slots)
**      30-Aug-96   floh    + schaltet Mauspointer in Cancel-Modus
**      02-Oct-96   floh    + Bildschirm wird gelöscht
**                          + AET_MasterVolume wird abgefragt und
**                            nach LID gesichert
**      03-Oct-96   floh    + SoundSamples sind nicht mehr Set-
**                            spezifisch und werden aus "levels:"
**                            geladen
**      13-Dec-96   floh    + initialisiert ywd->FrameCount mit 0
**      15-Jan-97   floh    + Cleanup-Mauspointer
**      26-Feb-97   floh    + Palette wird jetzt initialisiert, bevor
**                            das Set geladen wird, damit GfxObjekte
**                            mit Custom-Texturen die korrekten
**                            Farben haben.
**      31-May-97   floh    + yw_ShowDiskAccess()
**      26-Aug-97   floh    + große Teile ausgelagert nach
**                            yw_CommonLevelInit()
**      07-Oct-97   floh    + yw_CheckSector() Args haben sich geändert
*/
{
    struct ypaworld_data *ywd = INST_DATA(cl,o);
    struct LevelDesc ld;
    BOOL level_ok = FALSE;

    if (yw_CommonLevelInit(ywd,&ld,msg->level_num,msg->level_mode)) {
        if (yw_LoadTypMap(ywd,ld.typ_map)) {
            if (yw_LoadOwnMap(ywd,ld.own_map)) {
                if (yw_LoadHgtMap(ywd,ld.hgt_map)) {
                    if (yw_PlaceRobos(o,ywd,ld.num_robos,&(ld.robos[0]))) {
                        if (yw_LoadBlgMap(o,ywd,ld.blg_map)) {

                            ULONG secx,secy;

                            /*** folgende Inits nicht im Player-Modus ***/
                            if (ywd->Level->Mode != LEVELMODE_SEQPLAYER) {

                                /*** Level-File-Squads plazieren ***/
                                yw_InitSquads(ywd,ld.num_squads,
                                              &(ld.squad[0]));

                                /*** mitgenommene Vehikel plazieren ***/
                                yw_CreateBuddies(ywd);

                                /*** globaler Sector-Owner-Check ***/
                                for (secy=0; secy<ywd->MapSizeY; secy++) {
                                    for (secx=0; secx<ywd->MapSizeX; secx++) {
                                        struct Cell *sec;
                                        sec = &(ywd->CellArea[secy*ywd->MapSizeX + secx]);
                                        yw_CheckSector(ywd,sec,secx,secy,255,NULL);
                                    };
                                };

                                /*** Wundersteine initialisieren ***/
                                yw_InitWundersteins(o,ywd);

                                /*** BeamGates initialisieren ***/
                                yw_InitGates(ywd);
                                yw_InitSuperItems(ywd);

                                /*** More Energy-Init ***/
                                yw_NewEnergyCycle(ywd);
                            };

                            /*** GUI-Modul initialisieren ***/
                            if (yw_InitGUIModule(o,ywd)) {
                                level_ok = TRUE;
                            };
                        };
                    };
                };
            };
        };
    };
    if (!level_ok) _methoda(o, YWM_KILLLEVEL, NULL);
    return(level_ok);
}


