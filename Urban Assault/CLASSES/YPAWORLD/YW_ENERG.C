/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_energy.c,v $
**  $Revision: 38.21 $
**  $Date: 1998/01/06 16:18:34 $
**  $Locker:  $
**  $Author: floh $
**
**  Energy-Handling für ypaworld.class
**
**  (C) Copyright 1995 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "ypa/ypaworldclass.h"
#include "ypa/ypabactclass.h"

#include "yw_protos.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
/*-----------------------------------------------------------------*/
void yw_KillEnergyModule(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Killt Energie-Zeugs komplett und endgültig. Wird
**      nur im Fehlerfall aus yw_InitEnergyModule() oder
**      im Normalfall aus OM_DISPOSE aufgerufen.
**
**  INPUTS
**      ywd -> Ptr auf LID des Welt-Objects.
**
**  RESULTS
**      ---
**
**  CHANGED
**      20-Jul-95   floh    created
*/
{
    if (ywd->KraftWerks) {
        _FreeVec(ywd->KraftWerks);
        ywd->KraftWerks = NULL;
        ywd->FirstFreeKraftWerk = 0;
    };

    if (ywd->EnergyMap) {
        _FreeVec(ywd->EnergyMap);
        ywd->EnergyMap = NULL;
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
BOOL yw_InitEnergyModule(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Initialisiert Energie-Zeugs. Wird innerhalb OM_NEW
**      aufgerufen.
**
**  INPUTS
**      ywd -> Ptr auf LID des Welt-Objects.
**
**  RESULTS
**      TRUE    -> alles OkeeDokee
**      FALSE   -> no mem
**
**  CHANGED
**      20-Jul-95   floh    created
**      19-Sep-95   floh    Ooops, hier war noch ein Missing Return Value!
**      14-Oct-95   floh    die EnergyMap ist jetzt nur noch 64*64*2 Bytes
**                          groß, ein Eintrag in der Energy-Map sieht so
**                          aus:
**                              1 Byte Owner
**                              1 Byte Multiplikator
**                          Durch die neue Größe ergibt sich eine
**                          neue Offset-Ermittlung im Floodfiller.
**      15-Oct-95   floh    debugging...
*/
{
    /*** Energy-Map initialisieren ***/
    ywd->EnergyMap = (struct EMapElm *) 
                     _AllocVec(64*64*sizeof(struct EMapElm), 
                     MEMF_PUBLIC|MEMF_CLEAR);
    if (NULL == ywd->EnergyMap) {
        yw_KillEnergyModule(ywd);
        return(FALSE);
    };

    /*** KraftWerks-Array allokieren... ***/
    ywd->KraftWerks = (struct KraftWerk *)
                      _AllocVec(MAXNUM_KRAFTWERKS * sizeof(struct KraftWerk),
                                MEMF_PUBLIC|MEMF_CLEAR);

    /*** ...und initialisieren ***/
    if (ywd->KraftWerks) {
        LONG i;
        for (i=0; i<MAXNUM_KRAFTWERKS; i++) {
            yw_FreeKraftWerk(ywd,i);
        };
    } else {
        yw_KillEnergyModule(ywd);
        return(FALSE);
    };
    ywd->FirstFreeKraftWerk = 0;
    ywd->ActKraftWerk       = 0;

    /*** das war's ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void yw_UpdateEnergyMap(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Wie yw_NewEnergyCycle(), gleicht aber nur die
**      EnergyMap and die SektorMap an, nicht umgekehrt.
**      Es kam sonst nämlich zu unschönen BlackOuts in
**      der 2D-Karte, wenn ein neues Kraftwerk einen
**      yw_NewEnergyCycle erzwungen hat. yw_AllocKraftWerk
**      ruft jetzt stattdessen yw_UpdateEnergyMap() auf.
**
**  CHANGED
**      03-Feb-96   floh    created
**      13-Jun-96   floh    + Bugfix: jetzt auch korrekt bei
**                            Map-Größen != 64x64
*/
{
    ULONG i,x,y;
    struct Cell *sec;
    struct EMapElm *emap;

    sec  = ywd->CellArea;
    emap = ywd->EnergyMap;
    for (i=0,y=0; y<(ywd->MapSizeY); y++) {
        for (x=0; x<(ywd->MapSizeX); x++,i++) {
            emap[x].owner  = sec[i].Owner;
            emap[x].factor = 0;
        };
        emap += 64;
    };
    ywd->ActKraftWerk = 0;

    /*** das war's ***/
}

/*-----------------------------------------------------------------*/
LONG yw_AllocKraftWerk(struct ypaworld_data *ywd,
                       ULONG x, ULONG y,
                       ULONG factor)
/*
**  FUNCTION
**      Sucht einen freien Eintrag im KraftWerks-Array und
**      gibt dessen Nummer zurück, oder -1L falls das
**      Kraftwerks-Array voll ist.
**
**      Falls ein freier Eintrag gefunden wurde, wird diese
**      Kraftwerks-Struktur gleich ausgefüllt.
**
**
**  INPUTS
**      ywd     -> Ptr auf LID des Welt-Objects
**      x       -> X-Sektor-Koordinate des Kraftwerks-Sektors
**      y       -> Y-Sektor-Koordinate des Kraftwerks-Sektors
**      factor  -> "Stärke" dieses Kraftwerks
**
**  RESULTS
**      Index einer freien Kraftwerks-Struktur im Kraftwerks-Array,
**      oder -1L falls bereits MAXNUM_KRAFTWERKS existieren.
**
**      Falls alles glattgeht, sind im neuen Kraftwerks-Eintrag
**      die Sektor-Koordinaten und der Sektor-Pointer eingetragen.
**
**  CHANGED
**      19-Jul-95   floh    created
**      14-Oct-95   floh    neu: <factor>
**      29-Jan-96   floh    setzt jetzt im Sector:
**                              sec->WType  = WType_IsKraftwerk
**                              sec->WIndex = Index ins Kraftwerks-Array
**      30-Jan-96   floh    factor_backup wird ausgefüllt
**      03-Feb-96   floh    yw_AllocKraftWerk() erzwingt jetzt ein
**                          yw_NewEnergyCycle(), damit die Energie-Map
**                          für den Floodfiller auf dem neuesten Stand
**                          ist. Das war nämlich der Grund für die
**                          mysteriösen "NewKraftwerk-Abstürze".
*/
{
    LONG i;
    struct KraftWerk *kw_array = ywd->KraftWerks;
    struct Cell *sec = &(ywd->CellArea[y*ywd->MapSizeX+x]);
    BOOL in_the_middle = FALSE;

    /*** zuerst mal gucken, ob "zwischendrin" eins frei ist... ***/
    for (i=0; i<ywd->FirstFreeKraftWerk; i++) {
        if (kw_array[i].sector == NULL) {
            in_the_middle = TRUE;
            break;
        };
    };

    /*** nein, also nach hinten anbauen, wenn noch Platz ist ***/
    if (i < MAXNUM_KRAFTWERKS) {

        if (!in_the_middle) {
            ywd->FirstFreeKraftWerk = i+1;
        };
        kw_array[i].x = x;
        kw_array[i].y = y;
        kw_array[i].factor_backup = factor;
        kw_array[i].factor = factor;
        kw_array[i].sector = sec;

        sec->WType  = WTYPE_Kraftwerk;
        sec->WIndex = i;

        yw_UpdateEnergyMap(ywd);

        return(i);

    } else return(-1);
}

/*-----------------------------------------------------------------*/
void yw_FreeKraftWerk(struct ypaworld_data *ywd, LONG index)
/*
**  FUNCTION
**      Gibt eine vorher allokierte Position im Kraftwerks-Array
**      wieder frei.
**
**  INPUTS
**      ywd     -> Ptr auf LID des Welt-Objects
**      index   -> Index im Kraftwerks-Array, entweder Rückgabe-Wert von
**                 yw_AllocKraftWerk(), yw_FindKWbyCoords() oder
**                 yw_FindKWbySector()
**
**  RESULTS
**      ---
**
**  CHANGED
**      19-Jul-95   floh    created
**      29-Jan-96   floh    im zugehörigen Sektor wird jetzt das
**                          WType-Feld gelöscht (war WTYPE_Kraftwerk)
**      30-Jan-96   floh    etwas effizienter...
**      20-Oct-96   floh    + löscht das entsprechende Feld
**                            in der Building-Map
*/
{
    struct Cell *sec;

    if (sec = ywd->KraftWerks[index].sector) {

        ywd->KraftWerks[index].sector = NULL;
        if (index == (ywd->FirstFreeKraftWerk-1)) {
            ywd->FirstFreeKraftWerk--;
        };

        sec->WType  = 0;
        sec->WIndex = 0;

        /*** lösche es aus Building-Map ***/
        if (ywd->BuildMap) {

            struct VFMBitmap *blg_map;
            UBYTE *blg_body;
            ULONG sec_x = ywd->KraftWerks[index].x;
            ULONG sec_y = ywd->KraftWerks[index].y;

            _get(ywd->BuildMap,BMA_Bitmap,&blg_map);
            blg_body = (UBYTE *) blg_map->Data;
            blg_body[sec_y*blg_map->Width + sec_x] = 0;
        };
    };
}

/*-----------------------------------------------------------------*/
LONG yw_FindKWbyCoords(struct ypaworld_data *ywd, ULONG x, ULONG y)
/*
**  FUNCTION
**      Sucht im Kraftwerks-Array nach Kraftwerk mit den
**      angegebenen Sektor-Koordinaten.
**
**  INPUTS
**      ywd     -> Ptr auf LID des Welt-Objects
**      x       -> gegebene X-Sektor-Koordinate
**      y       -> gegebene Y-Sektor-Koordinate
**
**  RESULTS
**      Gefundener Index oder -1L falls nicht existent.
**
**  CHANGED
**      19-Jul-95   floh    created
*/
{
    LONG i;
    struct KraftWerk *kw_array = ywd->KraftWerks;

    for (i=0; i<ywd->FirstFreeKraftWerk; i++) {
        if ((kw_array[i].x == x) && (kw_array[i].y == y)) return(i);
    };
    return(-1);
}

/*-----------------------------------------------------------------*/
LONG yw_FindKWbyPtr(struct ypaworld_data *ywd, struct Cell *sector)
/*
**  FUNCTION
**      Sucht im Kraftwerks-Array nach Kraftwerk mit den
**      angegebenen Sektor-Pointer.
**
**  INPUTS
**      ywd     -> Ptr auf LID des Welt-Objects
**      sector  -> gültiger Sektor-Pointer
**
**  RESULTS
**      Gefundener Index oder -1L falls nicht existent.
**
**  CHANGED
**      19-Jul-95   floh    created
*/
{
    LONG i;
    struct KraftWerk *kw_array = ywd->KraftWerks;

    for (i=0; i<ywd->FirstFreeKraftWerk; i++) {
        if (kw_array[i].sector == sector) return(i);
    };
    return(-1);
}

/*-----------------------------------------------------------------*/
void yw_DoKraftWerk(struct ypaworld_data *ywd, ULONG kw_num)
/*
**  FUNCTION
**      Füllt die EnergyMap mit "Energie" mit dem Kraftwerk
**      als Mittelpunkt.
**
**  CHANGED
**      22-Jun-96   floh    created
*/
{
    WORD d,f;      // Wirkungs-Radius und Energie-Faktor
    LONG dx0,dy0,dx1,dy1;
    LONG store_dx0;
    struct EMapElm *em;
    LONG skip_line;

    /*** Position des Kraftwerks ***/
    LONG px = ywd->KraftWerks[kw_num].x;
    LONG py = ywd->KraftWerks[kw_num].y;

    UBYTE owner = ywd->KraftWerks[kw_num].sector->Owner;

    /*** ermittle Wirkungsradius (Optimierungsbedürftig!) ***/
    f = ywd->KraftWerks[kw_num].factor;
    d = 0;
    while ((f>>=1) > 0) d++;
    f = ywd->KraftWerks[kw_num].factor;

    /*** Start- und End-Entfernungen im Wirk-Bereich ***/
    dx0 = -d;
    dy0 = -d;
    dx1 = d+1;
    dy1 = d+1;

    /*** Clippen (mit Sicherheitsbereich) ***/
    if ((px + dx0)<1) dx0=1-px;
    if ((py + dy0)<1) dy0=1-py;
    if ((px + dx1) >= ywd->MapSizeX) dx1=ywd->MapSizeX-px-1;
    if ((py + dy1) >= ywd->MapSizeY) dy1=ywd->MapSizeY-py-1;

    /*** Anfangsposition in EnergyMap ***/
    em = &(ywd->EnergyMap[((py+dy0)*64)+(px+dx0)]);

    /*** und los... ***/
    store_dx0 = dx0;
    skip_line = 64 - (dx1 - dx0);
    for (dy0; dy0<dy1; dy0++) {
        for (dx0=store_dx0; dx0<dx1; dx0++) {

            /*** sqrt(dx*dx+dy*dy)/4.0 ***/
            BYTE dist = ywd->DistTable[(abs(dx0)<<6)+abs(dy0)];
            WORD act  = em->factor;
            WORD new  = f>>dist;

            if (em->owner == owner) {
                /*** eigener Sektor -> Energien aufaddieren ***/
                act += new;
                if (act > 255) act=255;
                em->factor = (UBYTE) act;
            };
            em++;
        };
        em += skip_line;
    };
}

/*-----------------------------------------------------------------*/
void yw_NewEnergyCycle(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Bereitet die EnergyMap auf den FloodFill-Zyklus vor:
**
**      (1) überträgt die in der EnergyMap eingetragenen
**          Multiplikatoren in die Sektor-Map
**      (2) Initialisiert die Owner-Einträge der EnergyMap mit
**          den aktuellen Ownern der "Real-World". Man beachte,
**          daß die Randsektoren immer auf Owner==0 stehen,
**          das ist gleichzeitig die natürliche Begrenzung
**          für den Floodfill!
**      (3) ywd->ActKraftWerk = 0
**
**  INPUTS
**      ywd -> Pointer auf LID des Welt-Objekts
**
**  RESULTS
**      ---
**
**  CHANGED
**      14-Oct-95   floh    created
**      15-Oct-95   floh    debugging...
**      13-Jun-96   floh    + Bugfix: jetzt auch korrekt bei
**                            Map-Größen != 64x64
*/
{
    ULONG i,x,y;
    struct Cell *sec;
    struct EMapElm *emap;

    sec  = ywd->CellArea;
    emap = ywd->EnergyMap;
    for (i=0,y=0; y<(ywd->MapSizeY); y++) {
        for (x=0; x<(ywd->MapSizeX); x++,i++) {
            emap[x].owner  = sec[i].Owner;
            sec[i].EnergyFactor = emap[x].factor;
            emap[x].factor = 0;
        };
        emap += 64;
    };
    ywd->ActKraftWerk = 0;

    /*** das war's ***/
}

/*-----------------------------------------------------------------*/
void yw_Energize(struct ypaworld_data *ywd, ULONG ftime)
/*
**  FUNCTION
**      Handelt eine Timeslice des Energie-Zyklus ab.
**      Dabei wird jeweils nur eine Atomic-Action auf einmal
**      durchgeführt, das heißt entweder:
**          (1) ein yw_NewEnergyCycle()
**          (2) ein FloodFill() auf 1 Kraftwerk
**
**  INPUTS
**      ywd     -> Ptr auf LID des Welt-Objects
**      ftime   -> aktuelle Frame-Time
**
**  CHANGED
**      24-Jul-95   floh    created
**      25-Jul-95   floh    debugging -> Ooops, hatte ganz vergessen,
**                          in der Schleife den Sektor-Pointer zu
**                          inkrementieren...
**      20-Feb-97   floh    Bugfix vom AF integriert (act > FirstFreeKraftWerk)
**      09-Jun-97   floh    sobald mindestens ein BeamGate offen ist, werden
**                          alle User-Kraftwerke lahmgelegt
**      19-Aug-97   floh    + und wieder aus... Kraftwerke gehen wieder,
**                            wenn Beamgate auf
*/
{
    /*** existieren überhaupt Kraftwerke??? ***/
    if (ywd->FirstFreeKraftWerk > 0) {

        /*** das sieht strange aus, funktioniert aber... :-) ***/
        ULONG act = ywd->ActKraftWerk;
        while ((NULL == ywd->KraftWerks[act].sector) &&
               (act < ywd->FirstFreeKraftWerk))
        {
            act++;
        };

        if (act >= ywd->FirstFreeKraftWerk) {
            /*** ein neuer Energie-Zyklus beginnt ***/
            yw_NewEnergyCycle(ywd);
        } else {
            /*** aktuelles Kraftwerk "füllen" ***/
            if (ywd->KraftWerks[act].factor>0) yw_DoKraftWerk(ywd,act);
            ywd->ActKraftWerk = act+1;
        };
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yw_ActivateWunderstein(struct ypaworld_data *ywd, 
                            struct Cell *sec,
                            ULONG which_gem)
/*
**  FUNCTION
**      Aktiviert einen "eroberten" Wunderstein, indem
**      das Prototype-Script geparst wird und der
**      Logmsg-File angezeigt (nyi).
**
**  INPUTS
**      ywd     - Ptr auf LID des Welt-Objects
**      sec     - Sektor, in dem der Wunderstein entdeckt wurde
**      which   - Nummer des Wundersteins (0..7)
**
**  RESULTS
**      Es wird eine Fehlermeldung ausgegeben, wenn das
**      Parsen schief ging, das nützt aber nix, weil das
**      mitten im Spiel passiert!
**
**  CHANGED
**      05-May-96   floh    created
**      24-May-96   floh    Liest Msg und schreibt sie
**                          per YWM_LOGMSG raus.
**      10-Aug-96   floh    Pri der Wunderstein-Logmsg auf
**                          64 angehoben
**      12-Jan-96   floh    Wunderstein-Message nicht mehr per
**                          externem Script.
**      21-Jun-97   floh    falls kein Aktions-Script definiert,
**                          wird der eingebundene <begin_action>/<end_action>
**                          Block geparst, das ist die neue bevorzugte
**                          Methode, wegen "All-In-One-Levelscripts".
**      05-Jul-97   floh    + WTYPE des Sektors wird mit WTYPE_DeadWunderstein
**                            gefüllt.
**      16-Dec-97   floh    + fuellt die Touchstone-Struktur aus
**      10-Feb-98   floh    + zeigt nur noch eine neutrale TECH UPGRADE
**                            Message an, nicht mehr den im LDF definierten
**                            Erklärungs-Text.
*/
{
    struct Wunderstein *gem = &(ywd->gem[which_gem]);
    struct logmsg_msg lm;
    UBYTE msg[256];

    /*** Touchstone-Struktur ausfuellen ***/
    ywd->touch_stone.gem = which_gem;
    ywd->touch_stone.time_stamp = ywd->TimeStamp;
    ywd->touch_stone.vp_num = 0;
    ywd->touch_stone.bp_num = 0;
    ywd->touch_stone.wp_num = 0;

    /*** Aktions-Script, oder embedded? ***/
    if (gem->script_name[0]) {
        /*** Prototype-Script einlesen ***/
        if (!yw_ParseProtoScript(ywd, gem->script_name)) {
            _LogMsg("yw_ActivateWunderstein: ERROR parsing script %s.\n",gem->script_name);
        };
    } else {
        /*** Embedded Information parsen ***/
        yw_ParseGemAction(ywd,gem->action_1st_line,gem->action_last_line);
    };

    /*** Erklaerungs-Message anzeigen ***/
    strcpy(msg,ypa_GetStr(ywd->LocHandle,STR_LMSG_WUNDERSTEIN,"TECHNOLOGY UPGRADE!\n"));
    lm.bact = NULL;
    lm.pri  = 65;
    lm.msg  = msg;
    if (gem->type != 0) lm.code = gem->type;
    else                lm.code = 0;
    _methoda(ywd->world,YWM_LOGMSG,&lm);

    /*** Wunderstein im Sektor deaktivieren ***/
    sec->WType  = WTYPE_DeadWunderstein;
}

/*-----------------------------------------------------------------*/
void yw_SetOwner(struct ypaworld_data *ywd,
                 struct Cell *sec,
                 ULONG sec_x, ULONG sec_y,
                 UBYTE owner)
/*
**  FUNCTION
**      Frontend für Änderung des Owner-Codes eines Sektors.
**      Kapselt die Benachrichtigung per YWM_NOTIFYHISTORYEVENT.
**
**  CHANGED
**      26-Aug-97   floh    created
**      09-Sep-97   floh    + Powerstation/Techupgrades jetzt auch per
**                            YWM_NOTIFYHISTORYEVENT
**      19-May-98   floh    + Techupgrades werden nicht mehr hier
**                            als Historyevent notified
*/
{
    if (sec->Owner != owner) {

        struct ypa_HistConSec hcs;

        /*** Nachricht an Debriefing History ***/
        hcs.cmd = YPAHIST_CONSEC;
        hcs.sec_x = sec_x;
        hcs.sec_y = sec_y;
        hcs.new_owner = owner;
        _methoda(ywd->world,YWM_NOTIFYHISTORYEVENT,&hcs);

        /*** Spezial-Sektor-Typ??? ***/
        switch(sec->WType) {
            case WTYPE_Kraftwerk:
                hcs.cmd = YPAHIST_POWERSTATION;
                _methoda(ywd->world,YWM_NOTIFYHISTORYEVENT,&hcs);
                break;
        };

        /*** Sector-Counter modifizieren ***/
        ywd->SectorCount[sec->Owner]--;
        ywd->SectorCount[owner]++;

        /*** Owner modifizieren ***/
        sec->Owner = owner;
    };
}

/*-----------------------------------------------------------------*/
void yw_NewOwner(struct ypaworld_data *ywd,
                 ULONG sec_x, ULONG sec_y,
                 struct Cell *sec,
                 struct energymod_msg *emm,
                 ULONG new_owner)
/*
**  FUNCTION
**      Zählt die Gesamt-Energie aller Bakterien im Sektor
**      zusammen und manipuliert evtl. den Owner.
**      Es zählen nur Bakterien innerhalb des Sektors!
**
**  CHANGED
**      14-Oct-95   floh    created
**      09-Sep-96   floh    Bugfix: Owner-0 Objekte wurden in den
**                          Eroberungs-Check mit einbezogen, deshalb
**                          wurde beim Sektor-Erobern manchmal auf
**                          Owner 0 geschaltet, weil die Explosions-
**                          Brocken mit beachtet wurden.
**      02-Nov-96   floh    updated SectorCount[] Array
**      26-Aug-97   floh    + Sektor-Koordinaten-Args
**      07-Oct-97   floh    + bei Kraftwerks-Eroberung wird eine
**                            Logmsg losgeschickt
**      26-May-98   floh    + Falls Owner == 255, wird ein Check gemacht,
**                            ansonsten wird der Owner einfach gesetzt.
**                            War notwendig, weil im Netzwerkspiel kein
**                            yw_NewOwner(), sondern yw_SetOwner() gemacht
**                            wurde, und deshalb diverse Voiceovers nicht
**                            kamen.
*/
{
    ULONG i;
    LONG energy[MAXNUM_OWNERS];
    struct MinNode *nd;
    struct MinList *ls;

    /*** falls Owner nicht zwangsgesetzt, per Energie ermitteln ***/
    if (new_owner == 255) {
        new_owner = sec->Owner;
        memset(energy,0,sizeof(energy));
        ls = &(sec->BactList);
        for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {
            struct Bacterium *b = (struct Bacterium *) nd;
            energy[b->Owner] += b->Energy;
        };

        /*** neutrale Objekte ignorieren ***/
        energy[0] = 0;

        /*** neuer Eigentümer? ***/
        for (i=0; i<MAXNUM_OWNERS; i++) {
            if (energy[i] > energy[new_owner]) new_owner = i;
        };
    };
    
    /*** irgendeine Meldung bei Owner-Wechsel? ***/
    if (sec->Owner != new_owner) {
        if (sec->WType == WTYPE_Kraftwerk) {
        
            /*** Powerstation lost/captured ***/        
            if (new_owner == ywd->URBact->Owner) {
                /*** User hat ein Kraftwerk eingenommen ***/
                if (emm && (emm->hitman)) {
                    struct logmsg_msg lm;
                    lm.bact = emm->hitman;
                    lm.pri  = 78;
                    lm.msg  = NULL;
                    lm.code = LOGMSG_POWER_CAPTURED;
                    _methoda(ywd->world,YWM_LOGMSG,&lm);
                };
            } else if (sec->Owner == ywd->URBact->Owner) {
                /*** User hat ein Kraftwerk verloren ***/
                struct logmsg_msg lm;
                lm.bact = NULL;
                lm.pri  = 78;
                lm.msg  = NULL;
                lm.code = LOGMSG_POWER_LOST;
                _methoda(ywd->world,YWM_LOGMSG,&lm);
            };
        } else {
            /*** ein Keysector? ***/
            ULONG i;
            for (i=0; i<ywd->Level->NumGates; i++) {
                struct Gate *g = &(ywd->Level->Gate[i]);
                ULONG j;
                for (j=0; j<g->num_keysecs; j++) {
                    struct KeySector *ks = &(g->keysec[j]);
                    if (ks->sec == sec) {
                        /*** Keysector lost/captured ***/
                        if (new_owner == ywd->URBact->Owner) {
                            struct logmsg_msg lm;
                            lm.bact = NULL;
                            lm.pri  = 80;
                            lm.msg  = NULL;
                            lm.code = LOGMSG_KEYSECTOR_CAPTURED;
                            _methoda(ywd->world,YWM_LOGMSG,&lm);
                        } else if (sec->Owner == ywd->URBact->Owner) {
                            struct logmsg_msg lm;
                            lm.bact = NULL;
                            lm.pri  = 80;
                            lm.msg  = NULL;
                            lm.code = LOGMSG_KEYSECTOR_LOST;
                            _methoda(ywd->world,YWM_LOGMSG,&lm);
                        };
                    };
                };
            };
        }; 
    };

    /*** SectorCount ausgleichen und Owner (evtl.) modifizieren ***/
    yw_SetOwner(ywd,sec,sec_x,sec_y,new_owner);
}

/*-----------------------------------------------------------------*/
void yw_CheckSector(struct ypaworld_data *ywd, 
                    struct Cell *sec,
                    ULONG sec_x, ULONG sec_y,
                    UBYTE owner,
                    struct energymod_msg *emm)
/*
**  FUNCTION
**      Testet, ob der übergebene Sektor den Empty-Status
**      erreicht hat (alle SubEnergien auf 0). Wenn dem
**      so ist, wird der Owner auf 0 gesetzt und
**      mit TRUE zurückgekehrt.
**
**  INPUTS
**      ywd -> LID des Welt-Objects
**      sec   -> Pointer auf Sektor
**      sec_x -> dessen X-Koordinate
**      sec_y -> dessen Y-Koordinate
**      emm   -> Pointer auf energymod_msg, welches mit einem
**               YWM_MODSECTORENERGY reinkam, oder NULL, wenn
**               Änderung nicht durch ModSectorEnergy.
**
**  CHANGED
**      10-Oct-95   floh    created
**      14-Oct-95   floh    jetzt mit Owner-Check
**      30-Jan-96   floh    Sonderbehandlung für Kraftwerke
**                          wird ein Kraftwerk auf 0 geschossen,
**                          wird es mit yw_FreeKraftWerk() gekillt,
**                          außerdem wird die Kraftwerks-Power
**                          neu mitskaliert.
**                          + Damnit! Da war noch so ein Scheiß
**                            Bug drin, der aus allen leeren Sektoren
**                            ein Kraftwerk gemacht hat...
**      17-May-96   floh    Neutrale Sektoren sind jetzt eroberbar,
**                          Randsektoren werden generell auf Owner 0
**                          gesetzt.
**      11-Sep-96   floh    Der Eroberungs-Threshold wurde bei
**                          Kompakt-Sektoren von 100 (max. 255)
**                          auf 192 (also 3/4 der Maximal-Energie)
**                          gesetzt, diese Justierung ist hauptsächlich
**                          für Kraftwerke sinnvoll.
**      08-Jul-97   floh    + neuer Parameter: vorgegebener Owner-Wert
**      02-Sep-97   floh    + Ooops, bei NewOwner() wurde zweimal
**                            <sec_y> übergeben...
**      07-Oct-97   floh    + <emm> Arg
**      23-Mar-98   floh    + Eroberungs-Threshold auf 224 für Kompakt-
**                            sektoren und 9*192 für 3x3 Sektoren
**                            gesetzt.
*/
{
    LONG e;
    ULONG sx,sy;

    if ((sec_x == 0) || (sec_y == 0) ||
        (sec_x == (ywd->MapSizeX-1)) ||
        (sec_y == (ywd->MapSizeY-1)))
    {
        /*** ein RandSektor, Owner auf 0 ***/
        yw_SetOwner(ywd,sec,sec_x,sec_y,0);
    } else {
        /*** Gesamt-Sektor-Energie berechnen ***/
        e = 0;
        for (sx=0; sx<3; sx++) {
            for (sy=0; sy<3; sy++) {
                e += sec->SubEnergy[sx][sy];
            };
        };
        /*** Sonderbehandlung-Kraftwerk: Power anpassen ***/
        if (WTYPE_Kraftwerk == sec->WType) {
            struct KraftWerk *kw = &(ywd->KraftWerks[sec->WIndex]);
            LONG new_factor;
            if (e == 0) {
                /*** Kraftwerk wurde zerstört ***/
                yw_FreeKraftWerk(ywd,sec->WIndex);
            } else {
                new_factor = (kw->factor_backup * e)/256;
                if (new_factor < 0)        new_factor = 0;
                else if (new_factor > 255) new_factor = 255;
                kw->factor = new_factor;
            };
        };

        /*** New-Owner-Check, wenn gewünscht ***/
        if (sec->SType == SECTYPE_COMPACT) {
            if (e < 224)   yw_NewOwner(ywd,sec_x,sec_y,sec,emm,owner);
        } else {
            if (e < 9*192) yw_NewOwner(ywd,sec_x,sec_y,sec,emm,owner);
        };
    };
}

/*-----------------------------------------------------------------*/
void yw_LegoExplodeFX(struct ypaworld_data *ywd,
                      struct Cell *sec,
                      LONG sec_x, LONG sec_y,
                      LONG sx, LONG sy,
                      ULONG lego)
/*
**  FUNCTION
**      Realisiert Explode-Effect für Legos. Es werden bis zu
**      8 VehicleProtos im ACTION_DEAD-Zustand erzeugt,
**      die dann vor sich hin explodieren und sich nach einer
**      Weile abschalten.
**
**  INPUTS
**      ywd     - LID-Pointer
**      sec     - betroffener Sector
**      sec_x,sec_y - dessen Sektor-Koordinaten
**      sx,sy       - Subsektor-Koords, [0,0] bei Kompact-Sektor
**
**  CHANGED
**      17-Jun-96   floh    created
**      17-Jul-96   floh    + Impuls-Parameter nach Bernds Wünschen
**                            geändert
**      07-Dec-96   floh    + umgestellt auf MAXNUM_EXPLODEFX, außerdem
**                            wird "ywd->NumDestFX" beachtet
**      11-Feb-98   floh    + neuer Parameter <lego>
*/
{
    LONG dx,dy,mx;

    /*** überhaupt im Sichtbereich? ***/
    dx = ywd->Viewer->SectX - sec_x;
    dy = ywd->Viewer->SectY - sec_y;
    dx = (dx < 0) ? -dx:dx;
    dy = (dy < 0) ? -dy:dy;
    if ((dx+dy) <= ((ywd->VisSectors-1)>>1)) {

        /*** ist schon mal potentiell sichtbar... ***/
        ULONG i;
        struct fp3d mp; // Mittelpunkt
        struct Lego *l = &(ywd->Legos[ ywd->Sectors[sec->Type].SSD[sx][sy]->limit_lego[lego] ]);

        mp.x = (FLOAT) (sec_x * SECTOR_SIZE + SECTOR_SIZE/2);
        mp.y = sec->Height;
        mp.z = (FLOAT) -(sec_y * SECTOR_SIZE + SECTOR_SIZE/2);
        if (SECTYPE_COMPACT != sec->SType) {
            mp.x += ((sx-1) * SLURP_WIDTH);
            mp.z += ((sy-1) * SLURP_WIDTH);
        };

        /*** bis zu 8 FX-Objects erzeugen ***/
        for (i=0; ((i < ywd->NumDestFX) && (l->fx_vp[i] != 0)); i++) {

            /*** dann mal lous... ***/
            Object *fxo;
            struct createvehicle_msg cvm;

            cvm.vp = l->fx_vp[i];
            cvm.x  = mp.x + l->fx_pos[i].x;
            cvm.y  = mp.y + l->fx_pos[i].y;
            cvm.z  = mp.z + l->fx_pos[i].z;
            fxo = (Object *) _methoda(ywd->world, YWM_CREATEVEHICLE, &cvm);
            if (fxo) {

                struct setstate_msg ssm;
                struct impulse_msg im;
                struct Bacterium *b;

                /*** FX-Object initialisieren ***/
                _get(fxo, YBA_Bacterium, &b);
                b->Owner = 0;

                ssm.main_state = ACTION_DEAD;
                ssm.extra_off  = 0;
                ssm.extra_on   = EXTRA_LOGICDEATH;
                _methoda(fxo, YBM_SETSTATE, &ssm);
                _methoda(ywd->world, YWM_ADDCOMMANDER, fxo);

                /*** einmaligen Impuls auf das Objekt ***/
                im.pos.x   = cvm.x;
                im.pos.y   = cvm.y;
                im.pos.z   = cvm.z;
                im.impulse = 40000;    // Schätzwert...
                im.miss.x  = l->fx_pos[i].x;    // als Richtungs-Vektor!
                im.miss.y  = -150;
                im.miss.z  = l->fx_pos[i].z;
                im.miss.v  = nc_sqrt(im.miss.x*im.miss.x +
                                     im.miss.y*im.miss.y +
                                     im.miss.z*im.miss.z);
                if (im.miss.v > 0.1) {
                    im.miss.x /= im.miss.v;
                    im.miss.y /= im.miss.v;
                    im.miss.z /= im.miss.v;
                };
                im.miss.v    = 30.0;
                im.miss_mass = 50.0;
                _methoda(fxo, YBM_IMPULSE, &im);
            };
        };
    };
}

/*-----------------------------------------------------------------*/
void yw_CheckUnderAttack(struct ypaworld_data *ywd, struct Cell *sec,
                         struct energymod_msg *msg) 
/*
**  FUNCTION
**      Testet, ob ein spezieller Sektor (Kraftwerke) under
**      Attack sind, und gibt eine Meldung aus (welche 
**      ausgebremst ist, damit nicht jeder Schuss eine 
**      Warnung ausgibt.
**
**  CHANGED
**      30-Apr-98   floh    created
**      26-May-98   floh    keine Meldung mehr bei eigenem Beschuss
*/
{
    if ((sec->WType == WTYPE_Kraftwerk)    && 
        (sec->Owner == ywd->URBact->Owner) &&
        (msg->hitman)                      &&
        (msg->hitman->Owner != ywd->URBact->Owner) &&
        ((ywd->TimeStamp - ywd->PowerAttackTimeStamp) > 5000))
    {
        struct logmsg_msg lm;
        ywd->PowerAttackTimeStamp = ywd->TimeStamp;
        lm.bact = NULL;
        lm.pri  = 77;
        lm.msg  = NULL;
        lm.code = LOGMSG_POWER_ATTACK;
        _methoda(ywd->world,YWM_LOGMSG,&lm);
    };
}

/*-----------------------------------------------------------------*/
LONG GET_SUB2(FLOAT x)
{
    /* ---------------------------------------------------------------------
    ** Wie das Makro, nur werden die Slurps den naechstliegenden SubSektoren
    ** zugesprochen. Rueckkehrwert liegt also zwischen 1 und 3, um den alten
    ** Slurptest zu passieren, allerdings muss dies bei der Sektorberechnung
    ** beachtet werden.
    ** -------------------------------------------------------------------*/
    LONG p;
    
    p = (LONG)(x/(SLURP_WIDTH/2));
    p = p % 8;
    if (p<3)      return(1);
    else if (p>4) return(3);
    else          return(2);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, yw_YWM_MODSECTORENERGY, struct energymod_msg *msg)
/*
**  FUNCTION
**      Energie-Modifikation eines Sektors. Energie darf
**      negativ (Beschuß) oder positiv (Reparatur) sein.
**      Alle Nebenwirkungen werden abgehandelt.
**
**  INPUTS
**      msg.pnt     -> absoluter 3d-Punkt, der den mehr oder weniger 
**                     genauen Punkt der Einwirkung beschreibt
**      msg.energy  -> abzuziehender Energie-Betrag in milliwatt, die
**                     Mindestenergie-Menge ist 1kW, sonst "verpufft"
**                     die Sache selbst bei minimalem Schild.
**
**  CHANGED
**      26-Jul-95   floh    created
**      10-Oct-95   floh    Die übergebene Energie wird jetzt
**                          umgerechnet in einen "Zerstörungsgrad-
**                          Modifikator", und direkt abgezogen.
**      14-Oct-95   floh    Randsektoren werden jetzt vollständig
**                          ignoriert.
**      24-Oct-95   floh    AF's Änderung übernommen, der Energie-
**                          Abzug findet jetzt nicht mehr in mW,
**                          sonder in Watt statt (hoffe ich jedenfalls)
**      30-Jan-96   floh    Sicher gegen "Negativ-Abzüge" gemacht
**      31-Jan-96   floh    wenn sich der betroffene Sektor im
**                          "Aufbau" befindet (WType == WTYPE_JobLocked),
**                          ist er gegen Energie-Modifikationen immun
**      28-Apr-96   floh    Gebäude jetzt generell etwas resistenter
**                          gegen Beschuß (Divisor von 250 auf 400).
**      05-May-96   floh    + Wunderstein Eroberung
**      17-May-96   floh    + Ooops, beim Check, ob die Sektor-Position
**                            gültig ist, wurde || statt && genommen...
**      20-Sep-96   floh    + im Designer-Modus ist die Routine
**                            wirkungslos
**      07-Oct-97   floh    + <energymod_msg> hat jetzt einen "Hitman"
**                            Parameter
**      11-Feb-98   floh    + bei sehr großen Energie-Einwirkungen, die
**                            mehrere Legos überspringen, werden jetzt
**                            alle Explosions-Stufen angezeigt.
**      18-May-98   floh    + neues Techupgrade History-Notifying
*/
{
    struct ypaworld_data *ywd = INST_DATA(cl,o);

    /*** Subkoordinaten des Ereignisses... ***/
    LONG secx =   msg->pnt.x  / SECTOR_SIZE;
    LONG secy = (-msg->pnt.z) / SECTOR_SIZE;

    #ifdef YPA_DESIGNMODE
    return;
    #endif

    /*** Subsektor-Position gültig? ***/
    if ((secx > 0) && (secx < (ywd->MapSizeX-1)) &&
        (secy > 0) && (secy < (ywd->MapSizeY-1)))
    {
        LONG sx,sy,inx,iny;
        LONG old_e,new_e,e_diff;
        LONG shield;

        /*** Sektor-Position und Sektor-Pointer ***/
        struct Cell *sec = &(ywd->CellArea[secy * ywd->MapSizeX + secx]);

        /*** In-Sektor-Position ***/
        inx  = GET_SUB2(   msg->pnt.x );
        iny  = GET_SUB2( -(msg->pnt.z) );

        /*** Slurp -> Abbruch ***/
        if ((inx==0)||(iny==0)) return;

        /*** Sektor im Aufbau? ***/
        if (WTYPE_JobLocked != sec->WType) {
            ULONG old_lego;
            ULONG new_lego;
            LONG lego_adder;

            /*** Array Index ermitteln ***/
            if (sec->SType == SECTYPE_COMPACT) {
                sx = 0;  sy = 0;
            } else {
                sx = inx-1;
                sy = 2 - (iny - 1);
            };

            /*** Schild, Energie und Energie-Differenz ***/
            shield = ywd->Legos[ GET_LEGONUM(ywd,sec,sx,sy) ].shield;
            old_e  = sec->SubEnergy[sx][sy];
            e_diff = (((100-shield)*msg->energy)/100)/400;

            new_e = old_e - e_diff;
            if      (new_e < 0)   new_e=0;
            else if (new_e > 255) new_e=255;

            /*** Explode-FX realisieren ***/
            old_lego = ywd->Energy2Lego[old_e];
            new_lego = ywd->Energy2Lego[new_e];
            if (old_lego < new_lego) {
                for (old_lego; old_lego<new_lego; old_lego++) {
                    yw_LegoExplodeFX(ywd,sec,secx,secy,sx,sy,old_lego);
                };
            } else if (old_lego > new_lego) {
                for (old_lego; old_lego>new_lego; old_lego--) {
                    yw_LegoExplodeFX(ywd,sec,secx,secy,sx,sy,old_lego);
                };
            };

            /*** neue Energie eintragen ***/
            sec->SubEnergy[sx][sy] = new_e;
            
            /*** UnderAttack Warnung? ***/
            yw_CheckUnderAttack(ywd, sec, msg);            

            /*** neuer Owner? ***/
            yw_CheckSector(ywd, sec, secx, secy, msg->owner, msg);

            /*** Wunderstein erobert??? ***/
            if (WTYPE_Wunderstein == sec->WType) {
                if (ywd->URBact && (ywd->URBact->Owner == sec->Owner)) {
                    /*** YAY! ***/
                    struct ypa_HistTechUpgrade htu;
                    if (ywd->playing_network) yw_ActivateNetWunderstein(ywd,sec,sec->WIndex);
                    else                      yw_ActivateWunderstein(ywd,sec,sec->WIndex);
                    
                    /*** Techupgrade als HistoryEvent registrieren ***/
                    htu.cmd       = YPAHIST_TECHUPGRADE;
                    htu.sec_x     = secx;
                    htu.sec_y     = secy;
                    htu.new_owner = sec->Owner;
                    htu.vp_num    = ywd->touch_stone.vp_num;
                    htu.wp_num    = ywd->touch_stone.wp_num;
                    htu.bp_num    = ywd->touch_stone.bp_num;
                    switch(ywd->gem[ywd->touch_stone.gem].type) {
                        case LOGMSG_TECH_WEAPON:            htu.type = YPAHIST_TECHTYPE_WEAPON; break;
                        case LOGMSG_TECH_ARMOR:             htu.type = YPAHIST_TECHTYPE_ARMOR;  break;
                        case LOGMSG_TECH_VEHICLE:           htu.type = YPAHIST_TECHTYPE_VEHICLE; break;
                        case LOGMSG_TECH_BUILDING:          htu.type = YPAHIST_TECHTYPE_BUILDING; break;
                        case LOGMSG_TECH_RADAR:             htu.type = YPAHIST_TECHTYPE_RADAR; break;
                        case LOGMSG_TECH_BUILDANDVEHICLE:   htu.type = YPAHIST_TECHTYPE_BUILDANDVEHICLE; break;
                        default:                            htu.type = YPAHIST_TECHTYPE_GENERIC; break;
                    };
                    _methoda(o,YWM_NOTIFYHISTORYEVENT,&htu);
                };
            } else if (WTYPE_DeadWunderstein == sec->WType) {
                if (ywd->playing_network) {
                    /*** falls Gesamtenergie auf 0, Wunderstein ***/
                    /*** endgültig killen                       ***/
                    LONG e = 0;
                    for (sx=0; sx<3; sx++) {
                        for (sy=0; sy<3; sy++) {
                            e += sec->SubEnergy[sx][sy];
                        };
                    };
                    if (e==0) yw_DisableNetWunderstein(ywd,sec,sec->WIndex);
                };
            };
        };
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yw_ComputeRatios(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Berechnet die Energie-Reload-Ratios für alle
**      Owner und schreibt diese in den Ratio-Cache.
**      Das ist effizienter, als die Sache jedesmal
**      von neuem in YWM_GETRLDRATIO zu machen.
**
**      Die Routine muß einmal am Anfang eines Frames
**      aufgerufen werden.
**
**  CHANGED
**      05-Nov-96   floh    created
*/
{
    LONG needed[8];
    ULONG i;

    /*** für jeden Owner Anzahl benötigter Kraftwerke ***/
    memset(needed,0,sizeof(needed));
    for (i=0; i<ywd->FirstFreeKraftWerk; i++) {
        struct KraftWerk *kw = &(ywd->KraftWerks[i]);
        if (kw->sector) needed[kw->sector->Owner] += kw->factor;
    };

    /*** und die Ratios... ***/
    for (i=0; i<8; i++) {
        needed[i] /= 2;
        if (needed[i] > 0) {
            LONG have = ywd->SectorCount[i];
            if (have >= needed[i]) {
                ywd->RatioCache[i] = 1.0;
                ywd->RoughRatio[i] = ((FLOAT)have)/((FLOAT)needed[i]);
                }
            else {
                ywd->RatioCache[i] = ((FLOAT)have)/((FLOAT)needed[i]);
                ywd->RoughRatio[i] = ((FLOAT)have)/((FLOAT)needed[i]);
                }
        } else {
            ywd->RatioCache[i] = 0.0;
            ywd->RoughRatio[i] = 0.0;
            }
    };

    /*** fertig ***/
}

/*-----------------------------------------------------------------*/
_dispatcher(void, yw_YWM_GETRLDRATIO, struct getrldratio_msg *msg)
/*
**  CHANGED
**      02-Nov-96   floh    created
*/
{
    struct ypaworld_data *ywd = INST_DATA(cl,o);
    msg->ratio       = ywd->RatioCache[msg->owner];
    msg->rough_ratio = ywd->RoughRatio[msg->owner];
}

/*-----------------------------------------------------------------*/
_dispatcher(void, yw_YWM_NOTIFYDEADROBO, struct notifydeadrobo_msg *msg)
/*
**  CHANGED
**      03-Nov-96   floh    created
**      26-Aug-97   floh    + updated für NotifyHistoryEvent
**      28-Feb-98   floh    + inkrementiert <ywd->Level->MaxNumBuddies>,
**                            falls der Killer der User-Robo war
*/
{
    struct ypaworld_data *ywd = INST_DATA(cl,o);
    struct MinList *ls;
    struct MinNode *nd;
    ULONG i,num_sec;
    struct Cell *sec;
    ULONG sec_x,sec_y;

    /*** Killer-ID 0 nicht erlaubt ***/
    if (msg->killer_id == 0) return;

    /*** falls Killer der User war, darf er 1 Buddy mehr mitnehmen ***/
    if (ywd->URBact && (ywd->URBact->Owner==msg->killer_id)) {
        ywd->Level->MaxNumBuddies++;
    };

    /*** existiert noch ein Robo gleichen Owners? ***/
    ls = &(ywd->CmdList);
    for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {

        struct OBNode *obnd = (struct OBNode *)nd;
        struct Bacterium *rbact = obnd->bact;

        if ((BCLID_YPAROBO == rbact->BactClassID) &&
            (msg->victim   != rbact) &&
            (msg->victim->Owner == rbact->Owner))
        {
            /*** Abbruch, weil noch ein gleich-owniger Robo ex. ***/
            return;
        };
    };

    /*** alles Land des gekillten fällt an Killer ***/
    sec = ywd->CellArea;
    for (sec_y=0; sec_y<ywd->MapSizeY; sec_y++) {
        for (sec_x=0; sec_x<ywd->MapSizeX; sec_x++) {
            if (sec->Owner == msg->victim->Owner) {
                yw_SetOwner(ywd,sec,sec_x,sec_y,msg->killer_id);
            };
            sec++;
        };
    };

    /*** falls der Vollstrecker der User-Robo war, fallen ***/
    /*** alle Wundersteine des Opfers an ihn              ***/
    if (ywd->URBact && (ywd->URBact->Owner==msg->killer_id)) {
        num_sec = ywd->MapSizeX * ywd->MapSizeY;
        sec = ywd->CellArea;
        for (sec_y=0; sec_y<ywd->MapSizeY; sec_y++) {
            for (sec_x=0; sec_x<ywd->MapSizeX; sec_x++) {
                if ((WTYPE_Wunderstein == sec->WType) &&
                    (sec->Owner == ywd->URBact->Owner))
                {
                    struct ypa_HistTechUpgrade htu;

                    /*** YAY! ***/
                    yw_ActivateWunderstein(ywd,sec,sec->WIndex);

                    /*** Techupgrade als HistoryEvent registrieren ***/
                    htu.cmd       = YPAHIST_TECHUPGRADE;
                    htu.sec_x     = sec_x;
                    htu.sec_y     = sec_y;
                    htu.new_owner = sec->Owner;
                    htu.vp_num    = ywd->touch_stone.vp_num;
                    htu.wp_num    = ywd->touch_stone.wp_num;
                    htu.bp_num    = ywd->touch_stone.bp_num;
                    switch(ywd->gem[ywd->touch_stone.gem].type) {
                        case LOGMSG_TECH_WEAPON:            htu.type = YPAHIST_TECHTYPE_WEAPON; break;
                        case LOGMSG_TECH_ARMOR:             htu.type = YPAHIST_TECHTYPE_ARMOR;  break;
                        case LOGMSG_TECH_VEHICLE:           htu.type = YPAHIST_TECHTYPE_VEHICLE; break;
                        case LOGMSG_TECH_BUILDING:          htu.type = YPAHIST_TECHTYPE_BUILDING; break;
                        case LOGMSG_TECH_RADAR:             htu.type = YPAHIST_TECHTYPE_RADAR; break;
                        case LOGMSG_TECH_BUILDANDVEHICLE:   htu.type = YPAHIST_TECHTYPE_BUILDANDVEHICLE; break;
                        default:                            htu.type = YPAHIST_TECHTYPE_GENERIC; break;
                    };
                    _methoda(o,YWM_NOTIFYHISTORYEVENT,&htu);
                };
                sec++;
            };
        };
    };

    /*** das war's schon ***/
}

