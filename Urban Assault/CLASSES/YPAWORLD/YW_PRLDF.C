/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_prldf.c,v $
**  $Revision: 38.2 $
**  $Date: 1998/01/06 16:25:41 $
**  $Locker: floh $
**  $Author: floh $
**
**  yw_prldf.c -- Level-Description-File-Parser.
**
**  (C) Copyright 1997 by A.Weissflog
*/
#include <exec/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "nucleus/nucleus2.h"

#include "ypa/ypaworldclass.h"

#include "yw_protos.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus

/*-----------------------------------------------------------------*/
ULONG yw_LevelDataParser(struct ScriptParser *p)
/*
**  FUNCTION
**      Parser-Funktion für <begin_level> Context-Blöcke, wie
**      sie in Level-Description-Files vorkommen.
**
**  INPUT
**      p->target   - Pointer auf auszufüllende <struct LevelDesc>
**      p->store[0] - struct ypaworld_data *ywd (Ptr auf LID des Welt-Objects)
**
**  CHANGED
**      03-May-96   floh    created
**      28-May-96   floh    lädt jetzt auch die GUI-Sounds...
**      12-Jun-96   floh    + Level-spez. Farbpalette
**      14-Aug-96   floh    - <palette> obsolete (wird aber noch akzeptiert)
**                          + slot0 .. slot7 -> Paletten-Slot-Keywords
**                            (<palette> entspricht <slot0>)
**      16-Oct-96   floh    + ywd->Level->Title wird per
**                            Keyword <title_default> bzw <title_language>
**                            ausgefüllt.
**      02-May-97   floh    + diverse Array- und String-Inits robuster
**                            gemacht
**      25-Oct-97   floh    + die GUI-Sound-Keywords sind raus
**      26-Jan-98   floh    + <movie> Keyword
**      19-Apr-98   floh    + <event_loop> Keyword
*/
{
    UBYTE *kw   = p->keyword;
    UBYTE *data = p->data;
    struct ypaworld_data *ywd = (struct ypaworld_data *) p->store[0];

    if (PARSESTAT_READY == p->status) {

        /*** momentan außerhalb eines Contexts ***/
        if (stricmp(kw, "begin_level")==0) {

            /*** ein paar defaults... ***/
            strcpy(ywd->Level->Title,"<NO NAME>");
            ywd->Level->Movie[0] = 0;
            p->status = PARSESTAT_RUNNING;
            return(PARSE_ENTERED_CONTEXT);

        } else return(PARSE_UNKNOWN_KEYWORD);

    } else {

        struct LevelDesc *ld = p->target;
        UBYTE title[32];

        /*** baue Keyword für Locale-spezifisches Title-Keyword ***/
        strcpy(title,"title_");
        strcat(title,ywd->LocaleLang);  // def="default"

        /*** momentan innerhalb eines Contexts ***/
        if (stricmp(kw,"end")==0){
            p->status = PARSESTAT_READY;
            return(PARSE_LEFT_CONTEXT);

        /*** title_#? ***/
        }else if (strstr(kw,"title_")){
            if ((stricmp(kw,"title_default")==0)||(stricmp(kw,title)==0)){
                if (strlen(data) >= (sizeof(ywd->Level->Title)-1)) {
                    _LogMsg("LevelDataParser(): Level titel too long!");
                    return(PARSE_BOGUS_DATA);
                };
                yw_LocStrCpy(ywd->Level->Title,data);
            };

        /*** set ***/
        }else if (stricmp(kw,"set")==0){
            ld->set_num = strtol(data,NULL,0);
            ld->flags |= LDESCF_SETNUM_OK;

        /*** sky ***/
        }else if (stricmp(kw,"sky")==0){
            if (strlen(data) >= (sizeof(ld->sky_name)-1)) {
                _LogMsg("LevelDataParser(): Sky name too long!");
                return(PARSE_BOGUS_DATA);
            };
            strcpy(ld->sky_name,data);
            ld->flags |= LDESCF_SKY_OK;

        /*** typ ***/
        }else if (stricmp(kw,"typ")==0){
            if (strlen(data) >= (sizeof(ld->typ_map)-1)) {
                _LogMsg("LevelDataParser(): Type map name too long!");
                return(PARSE_BOGUS_DATA);
            };
            strcpy(ld->typ_map,data);
            ld->flags |= LDESCF_TYPMAP_OK;

        /*** own ***/
        }else if (stricmp(kw,"own")==0){
            if (strlen(data) >= (sizeof(ld->own_map)-1)) {
                _LogMsg("LevelDataParser(): Owner map name too long!");
                return(PARSE_BOGUS_DATA);
            };
            strcpy(ld->own_map,data);
            ld->flags |= LDESCF_OWNMAP_OK;

        /*** hgt ***/
        }else if (stricmp(kw,"hgt")==0){
            if (strlen(data) >= (sizeof(ld->hgt_map)-1)) {
                _LogMsg("LevelDataParser(): Height map name too long!");
                return(PARSE_BOGUS_DATA);
            };
            strcpy(ld->hgt_map,data);
            ld->flags |= LDESCF_HGTMAP_OK;

        /*** blg ***/
        }else if (stricmp(kw,"blg")==0){
            if (strlen(data) >= (sizeof(ld->blg_map)-1)) {
                _LogMsg("LevelDataParser(): Building map name too long!");
                return(PARSE_BOGUS_DATA);
            };
            strcpy(ld->blg_map,data);
            ld->flags |= LDESCF_BLGMAP_OK;

        /*** palette ***/
        }else if (stricmp(kw,"palette")==0){
            strcpy(&(ld->pal_slot[0][0]),data);

        /*** slot0..7 ***/
        }else if (stricmp(kw,"slot0")==0)
            strcpy(&(ld->pal_slot[0][0]),data);
        else if (stricmp(kw,"slot1")==0)
            strcpy(&(ld->pal_slot[1][0]),data);
        else if (stricmp(kw,"slot2")==0)
            strcpy(&(ld->pal_slot[2][0]),data);
        else if (stricmp(kw,"slot3")==0)
            strcpy(&(ld->pal_slot[3][0]),data);
        else if (stricmp(kw,"slot4")==0)
            strcpy(&(ld->pal_slot[4][0]),data);
        else if (stricmp(kw,"slot5")==0)
            strcpy(&(ld->pal_slot[5][0]),data);
        else if (stricmp(kw,"slot6")==0)
            strcpy(&(ld->pal_slot[6][0]),data);
        else if (stricmp(kw,"slot7")==0)
            strcpy(&(ld->pal_slot[7][0]),data);

        /*** script ***/
        else if (stricmp(kw,"script")==0){
            struct ypaworld_data *ywd = (struct ypaworld_data *) p->store[0];
            if (!yw_ParseProtoScript(ywd, data)) return(PARSE_BOGUS_DATA);

        /*** ambiencetrack ***/
        }else if (stricmp(kw,"ambiencetrack")==0){

            /*** Default 0 (keine Verzoegerung ***/
            ywd->Level->ambience_min_delay = 0;
            ywd->Level->ambience_max_delay = 0;

            /*** Titelnummer ***/
            p = strtok( data, " \t_\n");
            ywd->Level->AmbienceTrack = strtol(p, NULL, 0);

            /*** evtl. noch Verzoegerungswerte ***/
            if( p = strtok( NULL, " \t_\n") ) {
                ywd->Level->ambience_min_delay = strtol(p, NULL, 0);
                if( p = strtok( NULL, " \t_\n") )
                    ywd->Level->ambience_max_delay = strtol(p, NULL, 0);
                }

        /*** Movie-Definitionen ***/
        }else if (stricmp(kw,"movie")==0)     strcpy(&(ywd->Level->Movie),data);
        else if (stricmp(kw,"win_movie")==0)  strcpy(&(ywd->Level->WinMovie),data);
        else if (stricmp(kw,"lose_movie")==0) strcpy(&(ywd->Level->LoseMovie),data);
        
        /*** event_loop ***/
        else if (stricmp(kw,"event_loop")==0) ld->event_loop = strtol(data,NULL,0);

        /*** slow_connection ***/
        else if (stricmp(kw,"slow_connection")==0) {

            if( (stricmp(data,"yes")==0) || (stricmp(data,"on")==0) ||
                (stricmp(data,"true")==0) )
                ld->slow_conn = TRUE;
            else
                ld->slow_conn = FALSE;
                 
        /*** UNKNOWN KEYWORD ***/
        } else return(PARSE_UNKNOWN_KEYWORD);

        /*** all-ok ***/
        return(PARSE_ALL_OK);
    };

    /*** can't happen ***/
    return(PARSE_UNKNOWN_KEYWORD);
}

/*-----------------------------------------------------------------*/
ULONG yw_LevelRoboParser(struct ScriptParser *p)
/*
**  FUNCTION
**      Parser-Funktion für <begin_robo> Context-Blöcke, wie
**      sie in Level-Description-Files vorkommen.
**
**  INPUT
**      p->target   - Pointer auf auszufüllende <struct LevelDesc>
**
**  CHANGED
**      03-May-96   floh    created
**      20-Jun-96   floh    - <sec_x,sec_y> obsolete, wird aber
**                            noch akzeptiert
**                          - <pos_x,pos_y,pos_z> neu.
**      16-Oct-96   floh    + <mb_status>
**      02-May-97   floh    + diverse Array- und String-Inits robuster
**                            gemacht
**      05-May-97   floh    + Robo-Positionen werden "künstlich" unrund
**                            gemacht
**      22-Apr-98   floh    + robo_reload_const
*/
{
    UBYTE *kw   = p->keyword;
    UBYTE *data = p->data;

    struct LevelDesc *ld = p->target;
    struct NLRoboDesc *robo = &(ld->robos[ld->num_robos]);

    if (PARSESTAT_READY == p->status) {

        /*** momentan außerhalb eines Contexts ***/
        if (stricmp(kw, "begin_robo")==0) {

            /*** Defaults eintragen ***/
            robo->mb_status = BRIEFSTAT_KNOWN;

            p->status   = PARSESTAT_RUNNING;
            return(PARSE_ENTERED_CONTEXT);
        } else return(PARSE_UNKNOWN_KEYWORD);

    } else {

        /*** momentan innerhalb eines Contexts ***/
        if (stricmp(kw,"end")==0) {
            p->status = PARSESTAT_READY;
            ld->num_robos++;
            ld->flags |= LDESCF_ROBOS_OK;
            return(PARSE_LEFT_CONTEXT);

        /*** owner ***/
        } else if (stricmp(kw,"owner")==0)
            robo->owner = strtol(data,NULL,0);

        /*** vehicle ***/
        else if (stricmp(kw,"vehicle")==0)
            robo->vhcl_proto = strtol(data,NULL,0);

        /*** sec_x ***/
        else if (stricmp(kw,"sec_x")==0){
            ULONG sec_x = strtol(data,NULL,0);
            FLOAT pos_x = (FLOAT) (sec_x * SECTOR_SIZE) + (SECTOR_SIZE/2);
            robo->pos.x = pos_x;
            robo->pos.y = -300.0;

        /*** sec_y ***/
        }else if (stricmp(kw,"sec_y")==0){
            ULONG sec_y = strtol(data,NULL,0);
            FLOAT pos_z = (FLOAT) -((sec_y * SECTOR_SIZE) + (SECTOR_SIZE/2));
            robo->pos.z = pos_z;
            robo->pos.y = -300.0;

        /*** pos_x ***/
        }else if (stricmp(kw,"pos_x")==0)
            robo->pos.x = atof(data) + 0.3;

        /*** pos_y ***/
        else if (stricmp(kw,"pos_y")==0)
            robo->pos.y = atof(data) + 0.3;

        /*** pos_z ***/
        else if (stricmp(kw,"pos_z")==0)
            robo->pos.z = atof(data) + 0.3;

        /*** energy ***/
        else if (stricmp(kw,"energy")==0)
            robo->energy = strtol(data,NULL,0);

        /*** con_budget ***/
        else if (stricmp(kw,"con_budget")==0)
            robo->conquer = strtol(data,NULL,0);

        /*** rad_budget ***/
        else if (stricmp(kw,"rad_budget")==0)
            robo->radar = strtol(data,NULL,0);

        /*** pow_budget ***/
        else if (stricmp(kw,"pow_budget")==0)
            robo->power = strtol(data,NULL,0);

        /*** def_budget ***/
        else if (stricmp(kw,"def_budget")==0)
            robo->defense = strtol(data,NULL,0);

        /*** neu: 8100 000C / safety_budget ***/
        else if (stricmp(kw,"saf_budget")==0)
            robo->safety = strtol(data,NULL,0);

        /*** neu 8100 000C / rec_budget ***/
        else if (stricmp(kw,"rec_budget")==0)
            robo->recon = strtol(data,NULL,0);

        /*** neu 8100 000C / cpl_budget ***/
        else if (stricmp(kw,"cpl_budget")==0)
            robo->place = strtol(data,NULL,0);

        /*** neu 8100 000C / rob_budget ***/
        else if (stricmp(kw,"rob_budget")==0)
            robo->robo = strtol(data,NULL,0);

        /*** neu 8100 000C / Blickrichtung ***/
        else if (stricmp(kw,"viewangle")==0)
            robo->viewangle = strtol(data,NULL,0);

        /*** neu: Anfangsverzoegerung fuer Jobs ***/
        else if (stricmp(kw,"saf_delay")==0)
            robo->saf_delay = strtol(data,NULL,0);

        /*** neu: Anfangsverzoegerung fuer Jobs ***/
        else if (stricmp(kw,"pow_delay")==0)
            robo->pow_delay = strtol(data,NULL,0);

        /*** neu: Anfangsverzoegerung fuer Jobs ***/
        else if (stricmp(kw,"rad_delay")==0)
            robo->rad_delay = strtol(data,NULL,0);

        /*** neu: Anfangsverzoegerung fuer Jobs ***/
        else if (stricmp(kw,"cpl_delay")==0)
            robo->cpl_delay = strtol(data,NULL,0);

        /*** neu: Anfangsverzoegerung fuer Jobs ***/
        else if (stricmp(kw,"def_delay")==0)
            robo->def_delay = strtol(data,NULL,0);

        /*** neu: Anfangsverzoegerung fuer Jobs ***/
        else if (stricmp(kw,"con_delay")==0)
            robo->con_delay = strtol(data,NULL,0);

        /*** neu: Anfangsverzoegerung fuer Jobs ***/
        else if (stricmp(kw,"rec_delay")==0)
            robo->rec_delay = strtol(data,NULL,0);

        /*** neu: Anfangsverzoegerung fuer Jobs ***/
        else if (stricmp(kw,"rob_delay")==0)
            robo->rob_delay = strtol(data,NULL,0);

        /*** mb_status ***/
        else if (stricmp(kw,"mb_status")==0){
            if (stricmp(data,"known")==0)        robo->mb_status=BRIEFSTAT_KNOWN;
            else if (stricmp(data,"unknown")==0) robo->mb_status=BRIEFSTAT_UNKNOWN;
            else if (stricmp(data,"hidden")==0)  robo->mb_status=BRIEFSTAT_HIDDEN;
            else return(PARSE_BOGUS_DATA);
            
        /*** reload_const ***/
        }else if (stricmp(kw,"reload_const")==0){
            robo->robo_reload_const = strtol(data,NULL,0);            

        /*** UNKNOWN KEYWORD ***/
        }else return(PARSE_UNKNOWN_KEYWORD);

        /*** all-ok ***/
        return(PARSE_ALL_OK);
    };

    /*** can't happen ***/
    return(PARSE_UNKNOWN_KEYWORD);
}

/*-----------------------------------------------------------------*/
ULONG yw_LevelGemParser(struct ScriptParser *p)
/*
**  FUNCTION
**      Parser für Wunderstein-Data im Level-Description-
**      File.
**      Die Routine füllt direkt die Elemente im Wunderstein-
**      Array der Welt-Klasse aus <ywd->gem[n]>.
**      in <p->store[0]> wird ein Pointer auf die LID der
**      Welt-Klasse erwartet.
**
**  INPUTS
**      p->target   - Pointer auf <struct LevelDesc>
**      p->store[0] - <struct ypaworld_data *>
**
**  CHANGED
**      04-May-96   floh    created
**      05-May-96   floh    + "building" keyword, sowie Abbruch, wenn
**                            keine sinnvollen Daten übergeben wurden.
**      16-Oct-96   floh    + <mb_status>
**      12-Jan-96   floh    + msg_#? (Message an User mit Locale-
**                            Support)
**      02-May-97   floh    + "log" Keyword wurde gekillt
**                          + diverse Array- und String-Inits robuster
**                            gemacht
**      20-Jun-97   floh    + externes Aktions-Script jetzt obsolete!
**                            stattdessen Keywords <begin_action><end_action>,
**                            die einen "Aktions-Script" kompatiblen
**                            Bereich im File umschließen, von diesem wird die
**                            1.und letzte Zeilennummer aufgehoben und
**                            ausgewertet, wenn der Wunderstein erobert wurde.
**      27-Sep-97   floh    + <type> Keyword
*/
{
    UBYTE *kw   = p->keyword;
    UBYTE *data = p->data;

    struct ypaworld_data *ywd = (struct ypaworld_data *) p->store[0];
    ULONG act_gem = p->store[1];
    struct Wunderstein *gem = &(ywd->gem[act_gem]);

    if (PARSESTAT_READY == p->status) {

        /*** momentan außerhalb eines Contexts ***/
        if (stricmp(kw, "begin_gem")==0) {

            int i;

            /*** Defaults eintragen ***/
            gem->mb_status = BRIEFSTAT_KNOWN;

            p->status   = PARSESTAT_RUNNING;
            p->store[2] = FALSE;    // derzeit nicht innerhalb des ACTION Block
            return(PARSE_ENTERED_CONTEXT);

            /*** prophilaktisch NetzProtos löschen ***/
            for( i = 0; i < 4; i++ )  {
                gem->nw_vproto_num[ i ] = 0;
                gem->nw_bproto_num[ i ] = 0;
                }

        } else return(PARSE_UNKNOWN_KEYWORD);

    } else {

        /*** momentan innerhalb eines Contexts ***/
        UBYTE msg_kw[32];

        if (act_gem > 7) return(PARSE_BOGUS_DATA);

        /*** baue Locale-spezifisches Msg-Keyword ***/
        strcpy(msg_kw,"msg_");
        strcat(msg_kw,ywd->LocaleLang);  // def="default"

        /*** falls gerade in einem <begin_action> Block, wird nur ***/
        /*** <end_action> ausgewertet!                            ***/
        if (p->store[2]) {
            if (stricmp(kw,"end_action")==0) {
                gem->action_last_line = p->line;
                p->store[2] = FALSE;    // außerhalb Action Block
            };
            return(PARSE_ALL_OK);
        };

        /*** end ***/
        if (stricmp(kw,"end")==0) {

            /*** Test, ob alles ok ***/
            if (0 == gem->bproto){
                _LogMsg("WStein init: gem[%d] no building defined!\n",act_gem);
                return(PARSE_BOGUS_DATA);
            };
            if ((0==gem->sec_x)||(0==gem->sec_y)){
                _LogMsg("WStein init: gem[%d] sector pos wonky tonk!\n",act_gem);
                return(PARSE_BOGUS_DATA);
            };

            gem->active = TRUE;

            p->status = PARSESTAT_READY;
            p->store[1]++;  // neuer "act_gem"
            return(PARSE_LEFT_CONTEXT);

        /*** msg_#? ***/
        }else if (strstr(kw,"msg_")){
            if ((stricmp(kw,"msg_default")==0)||(stricmp(kw,msg_kw)==0)){
                if (strlen(data) >= (sizeof(gem->msg)-1)) {
                    _LogMsg("LevelGemParser(): Msg too long!");
                    return(PARSE_BOGUS_DATA);
                };
                yw_LocStrCpy(gem->msg,data);
            };

        /*** sec_x ***/
        } else if (stricmp(kw,"sec_x")==0)
            gem->sec_x = strtol(data,NULL,0);

        /*** sec_y ***/
        else if (stricmp(kw,"sec_y")==0)
            gem->sec_y = strtol(data,NULL,0);

        /*** building ***/
        else if (stricmp(kw,"building")==0)
            gem->bproto = strtol(data,NULL,0);
        else if (stricmp(kw,"type")==0){
            switch(strtol(data,NULL,0)) {
                case 1:  gem->type = LOGMSG_TECH_WEAPON; break;
                case 2:  gem->type = LOGMSG_TECH_ARMOR; break;
                case 3:  gem->type = LOGMSG_TECH_VEHICLE; break;
                case 4:  gem->type = LOGMSG_TECH_BUILDING; break;
                case 5:  gem->type = LOGMSG_TECH_RADAR; break;
                case 6:  gem->type = LOGMSG_TECH_BUILDANDVEHICLE; break;
                default: gem->type = LOGMSG_TECH_GENERIC; break;
            };

        /*** script ***/
        }else if (stricmp(kw,"script")==0){

            /*** öffnen und schließen, als Test, ob vorhanden ***/
            APTR file;
            if (strlen(data) >= (sizeof(gem->script_name)-1)) {
                _LogMsg("LevelGemParser(): Script name too long!");
                return(PARSE_BOGUS_DATA);
            };
            strcpy(&(gem->script_name[0]),data);
            if (file = _FOpen(&(gem->script_name[0]),"r")) _FClose(file);
            else return(PARSE_BOGUS_DATA);

        /*** mb_status ***/
        }else if (stricmp(kw,"mb_status")==0){
            if (stricmp(data,"known")==0)        gem->mb_status=BRIEFSTAT_KNOWN;
            else if (stricmp(data,"unknown")==0) gem->mb_status=BRIEFSTAT_UNKNOWN;
            else if (stricmp(data,"hidden")==0)  gem->mb_status=BRIEFSTAT_HIDDEN;
            else return(PARSE_BOGUS_DATA);

        /*** Netzwerk-Vehicle ***/
        } else if (stricmp(kw,"nw_vproto_num")==0) {
            /*** VP's sind 4 zahlen, durch "_" getrennt, Reihenfolge: UKMT ***/
            char *d;
            if( d = strtok( data, "_ \t" ) ) {
                gem->nw_vproto_num[ 0 ] = strtol(d, NULL, 0);
                if( d = strtok( NULL, "_ \t" ) ) {
                    gem->nw_vproto_num[ 1 ] = strtol(d, NULL, 0);
                    if( d = strtok( NULL, "_ \t" ) ) {
                        gem->nw_vproto_num[ 2 ] = strtol(d, NULL, 0);
                        if( d = strtok( NULL, "_ \t" ) ) {
                            gem->nw_vproto_num[ 3 ] = strtol(d, NULL, 0);
                            }
                        }
                    }
                }

        /*** Netzwerk-Gebäude ***/
        } else if (stricmp(kw,"nw_bproto_num")==0) {
            /*** BP's sind 4 zahlen, durch "_" getrennt, Reihenfolge: UKMT ***/
            char *d;
            if( d = strtok( data, "_ \t" ) ) {
                gem->nw_bproto_num[ 0 ] = strtol(d, NULL, 0);
                if( d = strtok( NULL, "_ \t" ) ) {
                    gem->nw_bproto_num[ 1 ] = strtol(d, NULL, 0);
                    if( d = strtok( NULL, "_ \t" ) ) {
                        gem->nw_bproto_num[ 2 ] = strtol(d, NULL, 0);
                        if( d = strtok( NULL, "_ \t" ) ) {
                            gem->nw_bproto_num[ 3 ] = strtol(d, NULL, 0);
                            }
                        }
                    }
                }

        /*** neu: integrierter Aktions-Block ***/
        } else if (stricmp(kw,"begin_action")==0) {
            gem->action_1st_line = p->line;
            p->store[2] = TRUE;     // innerhalb Action Block

        /*** UNKNOWN KEYWORD?!?!?! ***/
        }else{
            /*** nur wenn innerhalb Action Block, alles erlaubt ***/
            if (!(p->store[2])) return(PARSE_UNKNOWN_KEYWORD);
        };

        /*** all-ok ***/
        return(PARSE_ALL_OK);
    };

    /*** can't happen ***/
    return(PARSE_UNKNOWN_KEYWORD);
}

/*-----------------------------------------------------------------*/
ULONG yw_LevelSquadParser(struct ScriptParser *p)
/*
**  FUNCTION
**      Parser für Squadron-Data (vorplazierte Geschwader
**      im Level).
**      Je Geschwader wird angegeben:
**
**              owner       -> welchem Robo gehört das Squad?
**              vehicle     -> VehicleProto für die Fahrzeuge
**              num         -> Anzahl der Vehicles
**              pos_x       -> direkte X-Position
**              pos_z       -> direkte Z-Position
**              useable     -> vorhanden: Robo darf Kommandos geben
**
**  INPUTS
**      p->target   - Pointer auf <struct LevelDesc>
**
**  CHANGED
**      25-May-96   floh    created
**      27-May-96   floh    + oops, p->status wurde bei "end"
**                            nicht auf PARSESTAT_READY geschaltet,
**                            deshalb wurde nur 1 Squadron akzeptiert.
**      16-Oct-96   floh    + <mb_status>
**      02-May-97   floh    + diverse Array- und String-Inits robuster
**                            gemacht
**      05-May-97   floh    + Squad-Init-Position wird künstlich
**                            unrund gemacht.
*/
{
    UBYTE *kw   = p->keyword;
    UBYTE *data = p->data;

    struct LevelDesc *ld = p->target;
    ULONG act_squad      = ld->num_squads;
    struct NLSquadDesc *squad = &(ld->squad[act_squad]);

    if (PARSESTAT_READY == p->status) {

        /*** momentan außerhalb eines Contexts ***/
        if (stricmp(kw, "begin_squad")==0) {

            /*** Defaults eintragen ***/
            squad->mb_status = BRIEFSTAT_KNOWN;

            p->status = PARSESTAT_RUNNING;
            return(PARSE_ENTERED_CONTEXT);
        } else return(PARSE_UNKNOWN_KEYWORD);

    } else {

        if (act_squad >= MAXNUM_SQUADS) return(PARSE_BOGUS_DATA);

        /*** momentan innerhalb eines Contexts ***/
        if (stricmp(kw,"end")==0) {

            /*** Test, ob alles ok ***/
            if (0 == squad->vproto) {
                _LogMsg("Squad init: squad[%d] no vehicle defined!\n",act_squad);
                return(PARSE_BOGUS_DATA);
            };
            if (0 == squad->num) {
                _LogMsg("Squad init: squad[%d] num of vehicles is 0!\n",act_squad);
                return(PARSE_BOGUS_DATA);
            };
            if ((0.0 == squad->pos.x) || (0.0 == squad->pos.z)) {
                _LogMsg("Squad init: squad[%d] no pos given!\n",act_squad);
                return(PARSE_BOGUS_DATA);
            };

            p->status = PARSESTAT_READY;
            squad->active = TRUE;
            ld->num_squads++;
            return(PARSE_LEFT_CONTEXT);

        /*** owner ***/
        } else if (stricmp(kw,"owner")==0)
            squad->owner = strtol(data,NULL,0);

        /*** useable ***/
        else if (stricmp(kw,"useable")==0)
            squad->useable = TRUE;

        /*** vehicle ***/
        else if (stricmp(kw,"vehicle")==0)
            squad->vproto = strtol(data,NULL,0);

        /*** num ***/
        else if (stricmp(kw,"num")==0)
            squad->num = strtol(data,NULL,0);

        /*** pos_x ***/
        else if (stricmp(kw,"pos_x")==0)
            squad->pos.x = atof(data) + 0.3;

        /*** pos_z ***/
        else if (stricmp(kw,"pos_z")==0)
            squad->pos.z = atof(data) + 0.3;

        /*** mb_status ***/
        else if (stricmp(kw,"mb_status")==0){
            if (stricmp(data,"known")==0)        squad->mb_status=BRIEFSTAT_KNOWN;
            else if (stricmp(data,"unknown")==0) squad->mb_status=BRIEFSTAT_UNKNOWN;
            else if (stricmp(data,"hidden")==0)  squad->mb_status=BRIEFSTAT_HIDDEN;
            else return(PARSE_BOGUS_DATA);

        /*** UNKNOWN KEYWORD ***/
        }else return(PARSE_UNKNOWN_KEYWORD);

        /*** all-ok ***/
        return(PARSE_ALL_OK);
    };

    /*** can't happen ***/
    return(PARSE_UNKNOWN_KEYWORD);
}

/*-----------------------------------------------------------------*/
ULONG yw_LevelGateParser(struct ScriptParser *p)
/*
**  FUNCTION
**      Scriptparser für BeamGate-Descriptions (Beamgates
**      sind die Level-Ausgänge).
**
**  CHANGED
**      19-Jun-96   floh    created
**      07-Aug-96   floh    + übernommen nach yw_level.c
**                          + Keysektoren, also Sektoren, die
**                            erobert/zerstört werden müssen,
**                            um ein spezielles BeamGate zu öffnen
**      16-Oct-96   floh    + <mb_status>
*/
{
    UBYTE *kw   = p->keyword;
    UBYTE *data = p->data;

    struct ypaworld_data *ywd = (struct ypaworld_data *) p->store[0];
    struct Gate *gate = &(ywd->Level->Gate[ywd->Level->NumGates]);

    if (PARSESTAT_READY == p->status) {

        /*** momentan außerhalb eines Contexts ***/
        if (stricmp(kw, "begin_gate")==0) {

            /*** Defaults eintragen ***/
            gate->mb_status = BRIEFSTAT_KNOWN;

            p->status = PARSESTAT_RUNNING;
            return(PARSE_ENTERED_CONTEXT);
        } else return(PARSE_UNKNOWN_KEYWORD);

    } else {

        if (ywd->Level->NumGates > MAXNUM_GATES) return(PARSE_BOGUS_DATA);

        /*** momentan innerhalb eines Context ***/
        if (stricmp(kw,"end")==0){

            /*** Test, ob alles OK ***/
            if (0 == gate->closed_bp) {
                _LogMsg("Gate init: gate[%d] no closed building defined!\n",ywd->Level->NumGates);
                return(PARSE_BOGUS_DATA);
            };
            if (0 == gate->opened_bp) {
                _LogMsg("Gate init: gate[%d] no opened building defined!\n",ywd->Level->NumGates);
                return(PARSE_BOGUS_DATA);
            };
            if ((0 == gate->sec_x) || (0 == gate->sec_y)) {
                _LogMsg("Gate init: gate[%d] no sector coords!\n",ywd->Level->NumGates);
                return(PARSE_BOGUS_DATA);
            };
            if (0 == gate->num_targets) {
                _LogMsg("Gate init: gate[%d] no target levels defined!\n",ywd->Level->NumGates);
                return(PARSE_BOGUS_DATA);
            };

            /*** nächstes Gate ***/
            ywd->Level->NumGates++;
            p->status = PARSESTAT_READY;
            return(PARSE_LEFT_CONTEXT);

        /*** sec_x ***/
        }else if (stricmp(kw,"sec_x")==0){
            gate->sec_x = strtol(data,NULL,0);

        /*** sec_y ***/
        }else if (stricmp(kw,"sec_y")==0){
            gate->sec_y = strtol(data,NULL,0);

        /*** closed_bp ***/
        }else if (stricmp(kw,"closed_bp")==0){
            gate->closed_bp = strtol(data,NULL,0);

        /*** opened_bp ***/
        }else if (stricmp(kw,"opened_bp")==0){
            gate->opened_bp = strtol(data,NULL,0);

        /*** target_level ***/
        }else if (stricmp(kw,"target_level")==0){
            if (gate->num_targets < MAXNUM_TARGETS){
                gate->targets[gate->num_targets++] = strtol(data,NULL,0);
            };

        /*** keysec_x ***/
        }else if (stricmp(kw,"keysec_x")==0) {
            if (gate->num_keysecs < MAXNUM_KEYSECS){
                gate->keysec[gate->num_keysecs].sec_x = strtol(data,NULL,0);
            };

        /*** keysec_y ***/
        }else if (stricmp(kw,"keysec_y")==0) {
            if (gate->num_keysecs < MAXNUM_KEYSECS){
                gate->keysec[gate->num_keysecs++].sec_y = strtol(data,NULL,0);
            };

        /*** mb_status ***/
        }else if (stricmp(kw,"mb_status")==0){
            if (stricmp(data,"known")==0)        gate->mb_status=BRIEFSTAT_KNOWN;
            else if (stricmp(data,"unknown")==0) gate->mb_status=BRIEFSTAT_UNKNOWN;
            else if (stricmp(data,"hidden")==0)  gate->mb_status=BRIEFSTAT_HIDDEN;
            else return(PARSE_BOGUS_DATA);

        /*** UNKNOWN KEYWORD ***/
        }else return(PARSE_UNKNOWN_KEYWORD);

        /*** all-ok ***/
        return(PARSE_ALL_OK);
    };

    /*** can't happen ***/
    return(PARSE_UNKNOWN_KEYWORD);
}

/*-----------------------------------------------------------------*/
ULONG yw_LevelSuperitemParser(struct ScriptParser *p)
/*
**  FUNCTION
**      Scriptparser für Superitem-Definitionen.
**
**  CHANGED
**      10-Feb-98   floh    created
*/
{
    UBYTE *kw   = p->keyword;
    UBYTE *data = p->data;

    struct ypaworld_data *ywd = (struct ypaworld_data *) p->store[0];
    struct SuperItem *item = &(ywd->Level->Item[ywd->Level->NumItems]);

    if (PARSESTAT_READY == p->status) {

        /*** momentan außerhalb eines Contexts ***/
        if (stricmp(kw, "begin_item")==0) {

            /*** Defaults eintragen ***/
            memset(item,0,sizeof(struct SuperItem));
            item->type      = SI_TYPE_NONE;
            item->time_diff = 60000;
            item->status    = SI_STATUS_INACTIVE;
            item->mb_status = BRIEFSTAT_KNOWN;
            p->status = PARSESTAT_RUNNING;
            return(PARSE_ENTERED_CONTEXT);
        } else return(PARSE_UNKNOWN_KEYWORD);

    } else {

        if (ywd->Level->NumItems > MAXNUM_SUPERITEMS) return(PARSE_BOGUS_DATA);

        /*** momentan innerhalb eines Context ***/
        if (stricmp(kw,"end")==0){

            /*** Test, ob alles OK ***/
            if ((0 == item->sec_x) || (0 == item->sec_y)) {
                _LogMsg("Super item #%d: invalid sector coordinates!\n",ywd->Level->NumItems);
                return(PARSE_BOGUS_DATA);
            };
            if (0 == item->inactive_bp) {
                _LogMsg("Super item #%d: no <inactive_bp> defined!\n",ywd->Level->NumItems);
                return(PARSE_BOGUS_DATA);
            };
            if (0 == item->active_bp) {
                _LogMsg("Super item #%d: no <active_bp> defined!\n",ywd->Level->NumItems);
                return(PARSE_BOGUS_DATA);
            };
            if (0 == item->trigger_bp) {
                _LogMsg("Super item #%d: no <trigger_bp> defined!\n",ywd->Level->NumItems);
                return(PARSE_BOGUS_DATA);
            };

            switch(item->type) {
                case SI_TYPE_BOMB:
                case SI_TYPE_WAVE:
                    /*** alles OK ***/
                    break;
                default:
                    /*** ooops, Fehler ***/
                    _LogMsg("Super item #%d: no valid <type> defined!\n",ywd->Level->NumItems);
                    return(PARSE_BOGUS_DATA);
            };

            /*** nächstes Item ***/
            ywd->Level->NumItems++;
            p->status = PARSESTAT_READY;
            return(PARSE_LEFT_CONTEXT);

        /*** sec_x ***/
        }else if (stricmp(kw,"sec_x")==0){
            item->sec_x = strtol(data,NULL,0);

        /*** sec_y ***/
        }else if (stricmp(kw,"sec_y")==0){
            item->sec_y = strtol(data,NULL,0);

        /*** inactive_bp ***/
        }else if (stricmp(kw,"inactive_bp")==0){
            item->inactive_bp = strtol(data,NULL,0);

        /*** active_bp ***/
        }else if (stricmp(kw,"active_bp")==0){
            item->active_bp = strtol(data,NULL,0);

        /*** trigger_bp ***/
        }else if (stricmp(kw,"trigger_bp")==0){
            item->trigger_bp = strtol(data,NULL,0);

        /*** keysec_x ***/
        }else if (stricmp(kw,"keysec_x")==0) {
            if (item->num_keysecs < MAXNUM_KEYSECS){
                item->keysec[item->num_keysecs].sec_x = strtol(data,NULL,0);
            };

        /*** keysec_y ***/
        }else if (stricmp(kw,"keysec_y")==0) {
            if (item->num_keysecs < MAXNUM_KEYSECS){
                item->keysec[item->num_keysecs++].sec_y = strtol(data,NULL,0);
            };

        /*** mb_status ***/
        }else if (stricmp(kw,"mb_status")==0){
            if (stricmp(data,"known")==0)        item->mb_status=BRIEFSTAT_KNOWN;
            else if (stricmp(data,"unknown")==0) item->mb_status=BRIEFSTAT_UNKNOWN;
            else if (stricmp(data,"hidden")==0)  item->mb_status=BRIEFSTAT_HIDDEN;
            else return(PARSE_BOGUS_DATA);

        /*** type ***/
        }else if (stricmp(kw,"type")==0){
            item->type = strtol(data,NULL,0);

        /*** countdown ***/
        }else if (stricmp(kw,"countdown")==0) {
            item->time_diff = strtol(data,NULL,0);

        /*** UNKNOWN KEYWORD ***/
        }else return(PARSE_UNKNOWN_KEYWORD);

        /*** all-ok ***/
        return(PARSE_ALL_OK);
    };

    /*** can't happen ***/
    return(PARSE_UNKNOWN_KEYWORD);
}

/*-----------------------------------------------------------------*/
ULONG yw_LevelEnableParser(struct ScriptParser *p)
/*
**  FUNCTION
**      Script-Parser für Vehicle/Building-Enabling.
**
**  INPUTS
**      p->target   - Pointer auf <struct LevelDesc>
**      p->store[0] - <struct ypaworld_data *>
**
**  CHANGED
**      20-Jun-97   floh    created
**      26-Jun-97   floh    + keine Aktion mehr bei Design-Modus
*/
{
    UBYTE *kw   = p->keyword;
    UBYTE *data = p->data;

    struct ypaworld_data *ywd = (struct ypaworld_data *) p->store[0];

    if (PARSESTAT_READY == p->status) {

        ULONG own = 0;
        ULONG i;

        /*** momentan außerhalb eines Contexts ***/
        if (stricmp(kw, "begin_enable")==0) {

            #ifndef YPA_DESIGNMODE
                /*** disable alle Vehicles/Buildings dieses Owners ***/
                own = strtol(data,NULL,0);
                for (i=0; i<NUM_VEHICLEPROTOS; i++) {
                    ywd->VP_Array[i].FootPrint &= ~(1<<own);
                };
                for (i=0; i<NUM_BUILDPROTOS; i++) {
                    ywd->BP_Array[i].FootPrint &= ~(1<<own);
                };
            #endif
            p->status   = PARSESTAT_RUNNING;
            p->store[1] = own;
            return(PARSE_ENTERED_CONTEXT);
        } else return(PARSE_UNKNOWN_KEYWORD);

    } else {

        ULONG own = p->store[1];

        /*** momentan innerhalb eines Context ***/
        if (stricmp(kw,"end")==0){
            p->status = PARSESTAT_READY;
            return(PARSE_LEFT_CONTEXT);

        } else if (stricmp(kw,"vehicle")==0) {
            #ifndef YPA_DESIGNMODE
                LONG vp_num = strtol(data,NULL,0);
                if ((vp_num<0)||(vp_num>=NUM_VEHICLEPROTOS)) return(PARSE_BOGUS_DATA);
                else ywd->VP_Array[vp_num].FootPrint |= (1<<own);
            #endif
        } else if (stricmp(kw,"building")==0) {
            #ifndef YPA_DESIGNMODE
                LONG bp_num = strtol(data,NULL,0);
                if ((bp_num<0)||(bp_num>=NUM_BUILDPROTOS)) return(PARSE_BOGUS_DATA);
                else ywd->BP_Array[bp_num].FootPrint |= (1<<own);
            #endif

        /*** Unknown Keyword ***/
        }else return(PARSE_UNKNOWN_KEYWORD);

        return(PARSE_ALL_OK);
    };

    /*** can't happen ***/
    return(PARSE_UNKNOWN_KEYWORD);
}

/*-----------------------------------------------------------------*/
void yw_ParseGemAction(struct ypaworld_data *ywd,
                       ULONG first_line,
                       ULONG last_line)
/*
**  FUNCTION
**      Parst den Wunderstein-Activate-Teil zwischen
**      <begin_action> und <end_action>.
**
**  CHANGED
**      21-Jun-97   floh    created
*/
{
    if (first_line != last_line) {

        APTR fp;
        UBYTE line[255];
        UBYTE *fname;

        /*** öffne Level-Description-File ***/
        fname = ywd->LevelNet->Levels[ywd->Level->Num].ldf;
        if (fp = _FOpen(fname,"r")) {

            ULONG line_num = 0;
            UBYTE old_path[256];
            struct ScriptParser parsers[3];
            ULONG res;

            /*** gehe auf 1.Zeile des Action-Blocks ***/
            while (++line_num <= first_line) _FGetS(line,sizeof(line),fp);

            /*** rufe Block-Parser solange auf, bis letzte Zeile erreicht ***/
            strcpy(old_path,_GetAssign("rsrc"));
            _SetAssign("rsrc","data:");

            memset(parsers,0,sizeof(parsers));
            parsers[0].parse_func = yw_VhclProtoParser;
            parsers[0].store[0]   = (ULONG) ywd;

            parsers[1].parse_func = yw_WeaponProtoParser;
            parsers[1].store[0]   = (ULONG) ywd;

            parsers[2].parse_func = yw_BuildProtoParser;
            parsers[2].store[1]   = (ULONG) ywd;

            /*** alle Kontextblöcke vor <end_action> parsen ***/
            do {
                res = yw_ParseContextBlock(fp,fname,3,parsers,&line_num,PARSEMODE_EXACT);
            } while ((res==PARSE_LEFT_CONTEXT) && (line_num<last_line));

            switch(res) {
                case PARSE_UNKNOWN_KEYWORD:
                    _LogMsg("GEM PARSE ERROR: Unknown Keyword, Script <%s> Line #%d\n",
                            fname, line_num);
                    break;
                case PARSE_BOGUS_DATA:
                    _LogMsg("GEM PARSE ERROR: Bogus Data, Script <%s> Line #%d\n",
                            fname, line_num);
                    break;
                case PARSE_UNEXPECTED_EOF:
                    _LogMsg("GEM PARSE ERROR: Unexpected EOF, Script <%s> Line #%d\n",
                            fname, line_num);
                    break;
            };

            /*** aufräumen ***/
            _SetAssign("rsrc",old_path);
            _FClose(fp);
        };
    };
}

/*-----------------------------------------------------------------*/
ULONG yw_MapParser(struct ScriptParser *p)
/*
**  FUNCTION
**      Parst einen <begin_map><end> Block und
**      lädt alle definierten Maps als Bitmap-Objekt
**      rein.
**
**  CHANGED
**      23-Jun-97   floh    created
**      26-Jan-98   floh    wenn TypeMap gelesen wird, wird
**                          deren Größe nach ld->size_x/size_y
**                          geschrieben
*/
{
    UBYTE *kw   = p->keyword;
    UBYTE *data = p->data;

    struct ypaworld_data *ywd = (struct ypaworld_data *) p->store[0];

    if (PARSESTAT_READY == p->status) {

        /*** momentan außerhalb eines Contexts ***/
        if (stricmp(kw, "begin_maps")==0) {
            /*** falls bereits Map-Objects existieren, diese killen ***/
            if (ywd->TypeMap)   { _dispose(ywd->TypeMap); ywd->TypeMap=NULL; };
            if (ywd->OwnerMap)  { _dispose(ywd->OwnerMap); ywd->OwnerMap=NULL; };
            if (ywd->HeightMap) { _dispose(ywd->HeightMap); ywd->HeightMap=NULL; };
            if (ywd->BuildMap)  { _dispose(ywd->BuildMap); ywd->BuildMap=NULL; };
            p->status   = PARSESTAT_RUNNING;
            return(PARSE_ENTERED_CONTEXT);
        } else return(PARSE_UNKNOWN_KEYWORD);

    } else {

        struct LevelDesc *ld = p->target;

        /*** momentan innerhalb eines Context ***/
        if (stricmp(kw,"end")==0){
            p->status = PARSESTAT_READY;
            return(PARSE_LEFT_CONTEXT);
        }else if (stricmp(kw,"typ_map")==0){
            ywd->TypeMap = yw_CreateBmpFromAscii(ywd,"typmap",p->fp);
            if (NULL == ywd->TypeMap) return(PARSE_BOGUS_DATA);
            _get(ywd->TypeMap,BMA_Width,&(ld->size_x));
            _get(ywd->TypeMap,BMA_Height,&(ld->size_y));
            ld->flags |= LDESCF_TYPMAP_OK;
        }else if (stricmp(kw,"own_map")==0){
            ywd->OwnerMap = yw_CreateBmpFromAscii(ywd,"ownmap",p->fp);
            if (NULL == ywd->OwnerMap) return(PARSE_BOGUS_DATA);
            ld->flags |= LDESCF_OWNMAP_OK;
        }else if (stricmp(kw,"hgt_map")==0){
            ywd->HeightMap = yw_CreateBmpFromAscii(ywd,"hgtmap",p->fp);
            if (NULL == ywd->HeightMap) return(PARSE_BOGUS_DATA);
            ld->flags |= LDESCF_HGTMAP_OK;
        }else if (stricmp(kw,"blg_map")==0){
            ywd->BuildMap = yw_CreateBmpFromAscii(ywd,"blgmap",p->fp);
            if (NULL == ywd->BuildMap) return(PARSE_BOGUS_DATA);
            ld->flags |= LDESCF_BLGMAP_OK;

        /*** Unknown Keyword ***/
        }else return(PARSE_UNKNOWN_KEYWORD);

        return(PARSE_ALL_OK);
    };

    /*** can't happen ***/
    return(PARSE_UNKNOWN_KEYWORD);
}

/*-----------------------------------------------------------------*/
void yw_GetSizeAndSkipAsciiBmp(APTR fp, ULONG *w_ptr, ULONG *h_ptr)
/*
**  FUNCTION
**      Liest die Breite und Höhe der AsciiBmp ein, und
**      überspringt die restlichen Zeilen.
**
**  CHANGED
**      26-Jan-98   floh    created
*/
{
    ULONG w = 0;
    ULONG h = 0;
    ULONG y;
    UBYTE *ptr;
    UBYTE line[1024];

    /*** Höhe und Breite lesen ***/
    _FGetS(line,sizeof(line),fp);
    ptr = strtok(line," \n");
    w   = strtol(ptr,NULL,0);
    ptr = strtok(NULL," \n");
    h   = strtol(ptr,NULL,0);

    /*** überspringe die restlichen Zeilen ***/
    for (y=0; y<h; y++) _FGetS(line,sizeof(line),fp);
    if (w_ptr) *w_ptr = w;
    if (h_ptr) *h_ptr = h;
}

/*-----------------------------------------------------------------*/
ULONG yw_MapSizeOnlyParser(struct ScriptParser *p)
/*
**  FUNCTION
**      Parst einen <begin_map><end> Block, lädt
**      die Maps aber NICHT, sondern nur deren Größe.
**
**  CHANGED
**      26-Jan-98   floh    created
*/
{
    UBYTE *kw   = p->keyword;
    UBYTE *data = p->data;

    struct ypaworld_data *ywd = (struct ypaworld_data *) p->store[0];

    if (PARSESTAT_READY == p->status) {

        /*** momentan außerhalb eines Contexts ***/
        if (stricmp(kw, "begin_maps")==0) {
            p->status   = PARSESTAT_RUNNING;
            return(PARSE_ENTERED_CONTEXT);
        } else return(PARSE_UNKNOWN_KEYWORD);

    } else {

        struct LevelDesc *ld = p->target;

        /*** momentan innerhalb eines Context ***/
        if (stricmp(kw,"end")==0){
            p->status = PARSESTAT_READY;
            return(PARSE_LEFT_CONTEXT);
        }else if (stricmp(kw,"typ_map")==0){
            ULONG w,h;
            yw_GetSizeAndSkipAsciiBmp(p->fp,&w,&h);
            ld->size_x = w;
            ld->size_y = h;
        }else if (stricmp(kw,"own_map")==0){
            yw_GetSizeAndSkipAsciiBmp(p->fp,NULL,NULL);
        }else if (stricmp(kw,"hgt_map")==0){
            yw_GetSizeAndSkipAsciiBmp(p->fp,NULL,NULL);
        }else if (stricmp(kw,"blg_map")==0){
            yw_GetSizeAndSkipAsciiBmp(p->fp,NULL,NULL);

        /*** Unknown Keyword ***/
        }else return(PARSE_UNKNOWN_KEYWORD);

        return(PARSE_ALL_OK);
    };

    /*** can't happen ***/
    return(PARSE_UNKNOWN_KEYWORD);
}


/*-----------------------------------------------------------------*/
LONG yw_GetLevelNumFromFilename(struct ypaworld_data *ywd, UBYTE *fname)
/*
**  FUNCTION
**      Versucht, Levelnummer aus Filename zu ermitteln,
**      der Filename darf keine Pfadkomponente enthalten!!!
**      Folgende Templates werden akzeptiert:
**
**          l1212.ldf       - Level 12
**          l123123.ldf     - Level 123
**          l12.ldf         - Level 12
**          l123.ldf        - Level 123
**
**  INPUTS
**      ywd     - Pointer auf <struct ypaworld_data>
**      fname   - Filename (ohne Pfad!)
**
**  RESULTS
**      Levelnummer, oder -1, falls Fehler.
**
**  CHANGED
**      26-Jan-98   floh    created
*/
{
    UBYTE name_buf[256];
    UBYTE *ptr;
    ULONG l;
    LONG lnum;

    /*** Levelnamen haben entweder den Aufbau lxy[xy].ldf oder lxyz[xyz].ldf ***/
    strcpy(name_buf,fname);
    ptr = strchr(name_buf,'.');
    if (ptr) *ptr=0;
    l = strlen(name_buf);
    switch(l) {
        case 5:
            /*** alter Doppel-2-Digit-Filename ***/
            name_buf[3] = 0;
            break;
        case 7:
            /*** alter Doppel-3-Digit-Filename ***/
            name_buf[4] = 0;
            break;
        default:
            /*** neumodischer Filename ***/
            break;
    };

    /*** Levelnummer ermitteln ***/
    lnum = atoi(&(name_buf[1]));
    if ((lnum < 1) || (lnum >= MAXNUM_LEVELS)) {
        _LogMsg("Invalid level num [valid: 0..127] for %s.\n",fname);
        lnum = -1;
    };
    return(lnum);
}

/*-----------------------------------------------------------------*/
BOOL yw_ReadLevelNodeInfo(struct ypaworld_data *ywd,
                          BOOL is_multiplayer,
                          UBYTE *fname)
/*
**  FUNCTION
**      Filtert Level-Nummer aus dem Filenamen, liest
**      alle relevanten Informationen aus dem LDF und schreibt
**      diese nach ywd->LevelNet->Levels[n]. Der Filename
**      darf KEINE Pathkomponente enthalten!!!
**
**      Die LevelNode Struktur wird KOMPLETT ausgefüllt.
**
**  CHANGED
**      26-Jan-98   floh    created
*/
{
    LONG lnum   = yw_GetLevelNumFromFilename(ywd,fname);
    BOOL retval = FALSE;
    if (lnum != -1) {

        struct LevelDesc ld;
        struct LevelNode *l = &(ywd->LevelNet->Levels[lnum]);
        UBYTE full_name[256];
        ULONG i;

        if (is_multiplayer) strcpy(full_name,"levels:multi/");
        else                strcpy(full_name,"levels:single/");
        strcat(full_name,fname);

        yw_ParseLDFLevelNodeInfo(ywd,&ld,full_name);
        l->status = is_multiplayer ? LNSTAT_NETWORK : LNSTAT_ENABLED;
        l->x0 = l->y0 = l->x1 = l->y1 = 0.0;
        memset(&(l->target),0,sizeof(l->target));
        strcpy(&(l->ldf),full_name);
        strcpy(&(l->title),&(ywd->Level->Title));
        memset(&(l->r),0,sizeof(l->r));
        l->num_players = ld.num_robos;
        l->races = 0;
        for (i=0; i<ld.num_robos; i++) {
            struct NLRoboDesc *r;
            r = &(ld.robos[i]);
            l->races |= ((UBYTE)(1<<r->owner));
        };
        l->size_x = ld.size_x;
        l->size_y = ld.size_y;
        l->slow_conn = ld.slow_conn;
        retval = TRUE;
    };
    return(retval);
}

/*-----------------------------------------------------------------*/
BOOL yw_ScanLevelDir(struct ypaworld_data *ywd,
                     BOOL is_multiplayer_dir)
/*
**  FUNCTION
**      Scannt ein Leveldir und macht für jeden
**      gefundenen Level ein yw_ReadLevelNodeInfo().
**
**  CHANGED
**      26-Jan-98   floh    created
*/
{
    struct ncDirEntry entry;
    APTR dir;
    UBYTE name[256];
    BOOL retval = TRUE;
    UBYTE *dir_name = is_multiplayer_dir ? "levels:multi/" : "levels:single/";

    if (dir = _FOpenDir(dir_name)) {
        while (_FReadDir(dir,&entry)) {

            UBYTE name[256];

            if ((strcmp(entry.name,".")  == 0) ||
                (strcmp(entry.name,"..") == 0))
            {
                continue;
            };
            if (yw_ReadLevelNodeInfo(ywd,is_multiplayer_dir,entry.name)){
                _LogMsg("Scanning [%s%s] .. ok.\n",dir_name,entry.name);
            }else{
                _LogMsg("Scanning [%s%s] .. FAILED.\n",dir_name,entry.name);
                retval=FALSE;
            };
        };
        _FCloseDir(dir);
    };
    return(retval);
}

/*-----------------------------------------------------------------*/
BOOL yw_ScanLevels(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Scannt die Single- und Multiplayer-Directories
**      und schreibt Informationen für alle gefundenen
**      Levels nach ywd->LevelNet->Levels[]. Das
**      Scannen passiert einmal beim Start des Spiels.
**
**  CHANGED
**      26-Jan-98   floh    created
*/
{
    BOOL retval = TRUE;

    /*** die SinglePlayer-Levels... ***/
    if (!yw_ScanLevelDir(ywd,FALSE)) retval = FALSE;

    /*** ...und die Multiplayer-Levels ***/
    if (!yw_ScanLevelDir(ywd,TRUE))  retval = FALSE;

    return(retval);
}




