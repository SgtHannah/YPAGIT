/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_parse.c,v $
**  $Revision: 38.15 $
**  $Date: 1998/01/06 16:25:06 $
**  $Locker: floh $
**  $Author: floh $
**
**  Diverse Script-Parse-Routinen (hauptsächlich die Routinen
**  zum Parsen der VehicleProto-/WeaponProto-/BuildProto-Scripts).
**
**  (C) Copyright 1996 by A.Weissflog
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
BOOL yw_PreParseLine(UBYTE *line, UBYTE **keyword, UBYTE **data)
/*
**  FUNCTION
**      Zerlegt einen String der Form
**
**          keyword [=] data    ; comment
**
**      in Keyword und Data. Kommentare starten mit
**      ; oder # und werden ignoriert.
**
**  INPUTS
**      str     - zu parsender String
**      keyword - Pointer wird auf Keyword umgebogen, oder NULL
**      data    - Pointer wird auf Dataword umgebogen, oder NULL
**
**  RESULTS
**      TRUE    - Keyword gültig
**      FALSE   - ungültige, oder leere Zeile
**
**  CHANGED
**      03-May-96   floh    created
**      09-Jun-96   floh    #-Kommentare removed
*/
{
    UBYTE *dike_out;

    /*** Kommentar + NewLine disablen ***/
    if (dike_out = strpbrk(line, ";\n")) *dike_out = 0;

    /*** Keyword isolieren ***/
    *keyword = strtok(line,"= \t");

    if (*keyword) *data = strtok(NULL, "= \t");
    else          return(FALSE);

    return(TRUE);
}

/*-----------------------------------------------------------------*/
BOOL yw_GetCookedLine(APTR fp,
                      UBYTE *kw_buf,
                      UBYTE *data_buf,
                      UBYTE **out_kw,
                      UBYTE **out_data,
                      ULONG *line_num_ptr)
/*
**  FUNCTION
**      Erweiterte Version von yw_PreParseLine(), kommt auch mit
**      Multiline-Statements zurecht, und ignoriert bereits
**      intern irgendwelche Leerzeilen...
**
**  INPUT
**      fp      - Filepointer des ASCII-Files
**      kw      - wird mit Pointer auf Keyword initialisiert
**      data    - wird mit Pointer auf Data initialisiert
**      multiline_buf   - Puffer für Keyword und Data
**      line_num_ptr    - Pointer auf Zeilen-Nummer
**
**  RESULT
**      TRUE    - Fileende noch nicht erreicht, bitte weiterparsen
**      FALSE   - EOF wurde erreicht
**
**  CHANGED
**      04-Aug-97   floh    created
**      13-Aug-97   floh    + bei Multiline-Sachen wird das Backslash
**                            durch ein '\n' ersetzt. Es kann also
**                            nur noch eingesetzt werden, wenn die
**                            Ausgabe-Routine damit klarkommt
*/
{
    UBYTE line[512];
    BOOL parse_next_line = TRUE;
    UBYTE *kw   = NULL;
    UBYTE *result = NULL;

    kw_buf[0]   = 0;
    data_buf[0] = 0;
    *out_kw   = NULL;
    *out_data = NULL;
    while (parse_next_line && (result = _FGetS(line,sizeof(line),fp))) {

        UBYTE *dike_out;
        UBYTE *data = NULL;

        /*** Zeilenzähler inkrementieren ***/
        (*line_num_ptr)++;

        /*** Kommentare + Newline killen ***/
        if (dike_out = strpbrk(line, ";\n")) *dike_out = 0;

        /*** wenn kw noch NULL, dann immer eine 1.Zeile ***/
        if (!kw) {
            kw = strtok(line,"= \t");
            if (kw) {
                /*** Keyword da, auch ein Data-Word? ***/
                strcpy(kw_buf,kw);
                data = strtok(NULL,"= \t");
            } else {
                /*** das war eine Leerzeile ***/
                data = NULL;
            };
        } else {
            /*** mitten in einer Multiline ***/
            data = strtok(line,"= \t");
        };

        /*** Data Fragment auswerten, oder komplette Multiline ***/
        if (data) {
            if (data[strlen(data)-1] == '\\') {
                /*** ein Multiline-Terminator! ***/
                data[strlen(data)-1] = '\n';
            } else {
                /*** letzte Zeile der Multiline ***/
                parse_next_line = FALSE;
            };
            strcat(data_buf,data);
        } else if (kw) {
            /*** eine Keyword- ohne Data-Zeile, parsen stoppen ***/
            parse_next_line = FALSE;
        };

        /*** Leer- oder Kommentarzeilen werden komplett ignoriert ***/
    };

    if (kw_buf[0])   *out_kw=kw_buf;
    if (data_buf[0]) *out_data=data_buf;
    return((BOOL)result);
}

/*-----------------------------------------------------------------*/
ULONG yw_ParseContextBlock(APTR fp,
                           UBYTE *fname,
                           ULONG num_parsers,
                           struct ScriptParser *parsers,
                           ULONG *line_num_ptr,
                           ULONG how)
/*
**  FUNCTION
**      Sucht Anfang eines neuen ContextBlocks und parst diesen.
**      Kehrt am Ende des ContextBlocks zurück, oder im Falle
**      eines Fehlers.
**      <include>'s werden transparent abgehandelt (diese dürfen
**      nur außerhalb von Kontextblöcken vorhanden sein und
**      auf denselben Script-Typ verweisen).
**
**  INPUTS
**      fp           - offener File
**      fname        - Filename des Files für Fehlermeldungen
**      num_parsers  - Anzahl Parsers im <parsers>-Array
**      parsers      - Array von initialisierten ScriptParser-Strukturen
**      line_num_ptr - zeigt auf ein initialisiertes ULONG mit der Line-Number
**      how          - PARSEMODE_EXACT, PARSEMODE_SLOPPY, PARSEMODE_NOINCLUDE
**
**  RESULTS
**      PARSE_LEFT_CONTEXT    - alles ok, weitere Contextblöcke könnten folgen
**      PARSE_UNKNOWN_KEYWORD - unbekanntes Keyword im Block oder außerhalb
**      PARSE_BOGUS_DATA      - "almost anything else"
**      PARSE_UNEXPECTED_EOF  - EOF innerhalb eines Kontext-Blocks
**      PARSE_EOF             - keine weiteren Kontext-Blöcke in diesem File
**
**  CHANGED
**      20-Jun-97   floh    created
**      04-Aug-97   floh    + benutzt jetzt yw_GetCookedLine(), kommt
**                            damit auch mit Multiline-Argumenten
**                            zurecht.
**      05-Aug-97   floh    + neues Arg: <how>
**      26-Jan-98   floh    + PARSEMODE_NOINCLUDE
*/
{
    UBYTE kw_buf[64];
    UBYTE data_buf[8*1024];
    UBYTE *kw,*data;
    struct ScriptParser *p = NULL;

    /*** nächste vorgekaute Zeile lesen ***/
    while (yw_GetCookedLine(fp,kw_buf,data_buf,&kw,&data,line_num_ptr)) {
        if (p) {
            /*** momentan innerhalb eines Kontextblock ***/
            ULONG result;
            p->keyword = kw;
            p->data    = data;
            p->line    = *line_num_ptr;
            p->fp      = fp;
            result     = p->parse_func(p);
            if (result != PARSE_ALL_OK) {
                /*** falls nicht PARSE_LEFT_CONTEXT, Fehlermeldung ***/
                switch (result) {
                    case PARSE_UNKNOWN_KEYWORD:
                        _LogMsg("PARSE ERROR: script %s line #%d unknown keyword %s.\n",
                                fname, *line_num_ptr, kw);
                        break;
                    case PARSE_BOGUS_DATA:
                        _LogMsg("PARSE ERROR: script %s line #%d data expected or bogus data.\n",
                                fname, *line_num_ptr);
                        break;
                };
                return(result);
            };

        } else if ((!(how & PARSEMODE_NOINCLUDE)) && (stricmp(kw,"include")==0)) {

            /*** ein <include> (außerhalb eines Kontextblock!) ***/
            if (!yw_ParseScript(data,num_parsers,parsers,how)) {
                _LogMsg("ERROR: script %s line #%d include %s failed!\n",
                        fname,*line_num_ptr,data);
                return(PARSE_BOGUS_DATA);
            }

        } else {

            /*** neuer Kontextblock?!? ***/
            ULONG i;
            for (i=0; i<num_parsers; i++) {
                /*** Parser auf Gültigkeit checken und init ***/
                struct ScriptParser *temp_p = &(parsers[i]);
                if (temp_p->parse_func) {
                    temp_p->status  = PARSESTAT_READY;
                    temp_p->line    = *line_num_ptr;
                    temp_p->keyword = kw;
                    temp_p->data    = data;
                    temp_p->fp      = fp;
                    /*** Parser fragen, ob bekannter Kontextheader ***/
                    if (temp_p->parse_func(temp_p) == PARSE_ENTERED_CONTEXT) {
                        /*** den nehmen wir ***/
                        p = temp_p;
                        i = num_parsers;
                    };
                };
            };
            /*** wenn hier noch kein Parser, dann unbekanntes Keyword! ***/
            if ((NULL == p) && (how & PARSEMODE_EXACT)) {
                _LogMsg("PARSE ERROR: script %s line #%d unknown keyword %s.\n",
                        fname, *line_num_ptr, kw);
                return(PARSE_UNKNOWN_KEYWORD);
            };
        };
    };

    /*** EOF aufgetreten, wat nu? ***/
    if (p) {
        _LogMsg("PARSE ERROR: script %s unexpected EOF!\n",fname);
        return(PARSE_UNEXPECTED_EOF);
    } else return(PARSE_EOF);
}

/*-----------------------------------------------------------------*/
BOOL yw_ParseScript(UBYTE *filename,
                    ULONG num_parsers,
                    struct ScriptParser *parsers,
                    ULONG how)
/*
**  FUNCTION
**      Universelle Scriptparser-Routine. Die Routine erledigt
**      die Routine-Arbeit und ruft Zeile für Zeile
**      die bereitgestellten Parser-Routinen auf. Jede
**      Parser-Routine muß in der Lage sein, ein spezielles
**      Context-Block-Format zu lesen (jeweils eingeleitet durch
**      ein spezielles begin_#? Keyword).
**      Context-Blöcke dürfen ***NICHT*** geschachtelt sein.
**
**  INPUTS
**      filename    - Filename des zu parsenden Scripts
**      num_parsers - Anzahl Parser
**      parsers     - Array von Parser-Beschreibungs-Strukturen,
**                    zumindest die <parse_func> muß initialisiert sein,
**                    außerdem je nach Parser <target>.
**      how         - PARSEMODE_EXACT: unbekannte Kontextblöcke
**                    führen zum Abbruch
**                    PARSEMODE_SLOPPY: unbekannte Kontextblöcke
**                    werden ignoriert
**
**  RESULTS
**      TRUE    -> alles OK
**      FALSE   -> ein Fehler ist aufgetreten
**
**  CHANGED
**      03-May-96   floh    created
**      06-May-96   floh    <include> Keyword
**      20-Jun-97   floh    ruft jetzt explizit ParseContextBlock
**                          auf
**      05-Aug-97   floh    + neues Arg: PARSE_SCRIPT_EXACTLY,
**                            oder PARSE_SCRIPT_SLOPPY
**      06-Aug-97   floh    + Bug: falls Script nicht geöffnet werden
**                            konnte, kehrte die Routine TRUE zurück
*/
{
    APTR fp;
    BOOL all_ok = TRUE;

    /*** Script-File öffnen ***/
    if (fp = _FOpen(filename, "r")) {

        ULONG line_num = 0;
        ULONG res;

        /*** yw_ParseContextBlock() aufrufen bis nix mehr geht ***/
        do {
            res = yw_ParseContextBlock(fp,filename,num_parsers,parsers,&line_num,how);
        } while (res == PARSE_LEFT_CONTEXT);

        /*** ernsthafte Fehler abfangen ***/
        switch(res) {
            case PARSE_UNKNOWN_KEYWORD:
            case PARSE_BOGUS_DATA:
            case PARSE_UNEXPECTED_EOF:
                all_ok = FALSE;
                break;
        };

        /*** Fehlermeldungen bereits in yw_ParseContextBlock()! ***/
        _FClose(fp);
    } else {
        /*** Script existiert nicht ***/
        all_ok = FALSE;
    };

    /*** Ende ***/
    return(all_ok);
}

/*-----------------------------------------------------------------*/
BOOL yw_ParseExtSampleDef(struct SoundInit *si, UBYTE *data)
/*
**  FUNCTION
**      Parsed eine Extended-Sample-Definition der
**      Form:
**
**          data_str = <loop>_<vol>_<pitch>_<off>_<len>_<wavname>
**
**      Das Ergebnis wird in das nächste freie SampleFragment
**      übertragen.
**      Falls alle Fragmente voll sind, macht die
**      Routine stillschweigend nichts.
**
**  CHANGED
**      20-Dec-96   floh    created
**      02-May-97   floh    + String-Init robuster
*/
{
    UBYTE *loop_str;
    UBYTE *vol_str;
    UBYTE *pitch_str;
    UBYTE *off_str;
    UBYTE *len_str;
    UBYTE *wav_str;

    /*** Data-String aufsplitten ***/
    loop_str  = strtok(data,"_");
    vol_str   = strtok(NULL,"_");
    pitch_str = strtok(NULL,"_");
    off_str   = strtok(NULL,"_");
    len_str   = strtok(NULL,"_");
    wav_str   = strtok(NULL,"_");
    if (loop_str && vol_str && pitch_str && off_str && len_str && wav_str) {

        /*** alles da, also los ***/
        if (si->ext_smp.count < MAX_FRAGMENTS) {

            ULONG i = si->ext_smp.count++;
            struct SampleFragment *sf = &(si->ext_smp.frag[i]);

            sf->sample = NULL;  // Sample-Name interessiert! (siehe unten)
            sf->loop   = strtol(loop_str,NULL,0);
            sf->volume = strtol(vol_str,NULL,0);
            sf->pitch  = strtol(pitch_str,NULL,0);
            sf->offset = strtol(off_str,NULL,0);
            sf->len    = strtol(len_str,NULL,0);
            if (strlen(wav_str) >= (sizeof(si->ext_smp_name[i])-1)) {
                _LogMsg("ParseExtSampleDef(): Name too long!\n");
                return(FALSE);
            };
            strcpy(si->ext_smp_name[i],wav_str);
        };
    } else return(FALSE);

    /*** wenn hier angekommen, alles in Ordnung ***/
    return(TRUE);
}

/*=================================================================**
**                                                                 **
**  FRONTEND SCRIPT PARSING ROUTINEN                               **
**                                                                 **
**=================================================================*/

/*-----------------------------------------------------------------*/
BOOL yw_ParseProtoScript(struct ypaworld_data *ywd, UBYTE *script)
/*
**  FUNCTION
**      Sammel-Parser für
**          - Vehicle Proto Descriptions (per yw_VhclProtoParser())
**          - Weapon Proto Descriptions (per yw_WeaponProtoParser())
**          - Build Proto Descriptions (per yw_BuildProtoParser())
**
**  INPUTS
**      ywd     - Ptr auf LID des Welt-Objects
**      script  - Filename des zu parsenden Scripts
**
**  RESULTS
**      TRUE
**      FALSE
**
**  CHANGED
**      03-May-96   floh    created
**      24-Apr-97   floh    + _SetSysPath()
*/
{
    UBYTE old_path[256];
    struct ScriptParser parsers[3];
    BOOL result;
    memset(parsers, 0, sizeof(parsers));

    strcpy(old_path,_GetAssign("rsrc"));
    _SetAssign("rsrc","data:");

    parsers[0].parse_func = yw_VhclProtoParser;
    parsers[0].store[0]   = (ULONG) ywd;

    parsers[1].parse_func = yw_WeaponProtoParser;
    parsers[1].store[0]   = (ULONG) ywd;

    parsers[2].parse_func = yw_BuildProtoParser;
    parsers[2].store[1]   = (ULONG) ywd;

    result = yw_ParseScript(script, 3, parsers, PARSEMODE_EXACT);
    _SetAssign("rsrc",old_path);
    return(result);
}

/*-----------------------------------------------------------------*/
BOOL yw_ParseLDF(struct ypaworld_data *ywd,
                 struct LevelDesc *ld,
                 UBYTE *script)
/*
**  FUNCTION
**      Lädt einen Level-Description-File.
**      Folgende Kontext-Blöcke werden akzeptiert:
**
**          <begin_level>   -> yw_LevelDataParser()
**          <begin_robo>    -> yw_LevelRoboParse()
**          <begin_gem>     -> yw_LevelGemParse()
**          <begin_squad>   -> yw_LevelSquadParser()
**          <new_vehicle>   -> yw_VhclProtoParser()
**          <new_weapon>    -> yw_WeaponProtoParser()
**          <new_building>  -> yw_BuildProtoParser()
**          <begin_enable>  -> yw_LevelEnableParser()
**
**  INPUTS
**      ywd     -> Ptr auf LID des Welt-Objects
**      ld      -> diese Level-Description-Struktur wird ausgefüllt
**      script  -> Filename des Level-Description-Scripts
**
**  RESULTS
**      TRUE/FALSE
**
**  CHANGED
**      03-May-96   floh    created
**      04-May-96   floh    + Wunderstein-Parser
**      25-May-96   floh    + Squadron-Parser
**      19-Jun-96   floh    + Gate-Parser
**      16-Oct-96   floh    + MissionMap-Parser
**      20-Jun-97   floh    + Vehicle/Weapon/BuildProto-Parser
**                          + Enable-Parser
**      24-Jun-97   floh    + yw_MapParser()
**      09-Sep-97   floh    + yw_DebriefingMapParser()
**      10-Feb-98   floh    + yw_LevelSuperitemParser()
*/
{
    BOOL result;
    struct ScriptParser parsers[13];
    memset(parsers, 0, sizeof(parsers));
    memset(ld, 0, sizeof(struct LevelDesc));

    parsers[0].parse_func = yw_LevelDataParser;
    parsers[0].target     = ld;
    parsers[0].store[0]   = (ULONG) ywd;

    parsers[1].parse_func = yw_LevelRoboParser;
    parsers[1].target     = ld;

    parsers[2].parse_func = yw_LevelGemParser;
    parsers[2].target     = ld;
    parsers[2].store[0]   = (ULONG) ywd;

    parsers[3].parse_func = yw_LevelSquadParser;
    parsers[3].target     = ld;

    parsers[4].parse_func = yw_LevelGateParser;
    parsers[4].target     = ld;
    parsers[4].store[0]   = (ULONG) ywd;

    parsers[5].parse_func = yw_MissionMapParser;
    parsers[5].target     = ld;

    parsers[6].parse_func = yw_VhclProtoParser;
    parsers[6].store[0]   = (ULONG) ywd;

    parsers[7].parse_func = yw_WeaponProtoParser;
    parsers[7].store[0]   = (ULONG) ywd;

    parsers[8].parse_func = yw_BuildProtoParser;
    parsers[8].store[1]   = (ULONG) ywd;

    parsers[9].parse_func = yw_LevelEnableParser;
    parsers[9].store[0]   = (ULONG) ywd;

    parsers[10].parse_func = yw_MapParser;
    parsers[10].target     = ld;
    parsers[10].store[0]   = (ULONG) ywd;

    parsers[11].parse_func = yw_DebriefingMapParser;
    parsers[11].target     = ld;

    parsers[12].parse_func = yw_LevelSuperitemParser;
    parsers[12].target     = ld;
    parsers[12].store[0]   = (ULONG) ywd;

    result = yw_ParseScript(script,13,parsers,PARSEMODE_EXACT);

    #ifdef YPA_DESIGNMODE
        strcpy(ywd->TypeMapName,(UBYTE *)&(ld->typ_map));
        strcpy(ywd->OwnerMapName,(UBYTE *)&(ld->own_map));
        strcpy(ywd->BuildMapName,(UBYTE *)&(ld->blg_map));
        strcpy(ywd->HeightMapName,(UBYTE *)&(ld->hgt_map));
    #endif

    return(result);
}

/*-----------------------------------------------------------------*/
BOOL yw_ParseLDFLevelNodeInfo(struct ypaworld_data *ywd,
                              struct LevelDesc *ld,
                              UBYTE *script)
/*
**  FUNCTION
**      Lädt einen Level-Description-File.
**      Folgende Kontext-Blöcke werden akzeptiert:
**
**          <begin_level>   -> yw_LevelDataParser()
**          <begin_robo>    -> yw_LevelRoboParse()
**
**  INPUTS
**      ywd     -> Ptr auf LID des Welt-Objects
**      ld      -> diese Level-Description-Struktur wird ausgefüllt
**      script  -> Filename des Level-Description-Scripts
**
**  RESULTS
**      TRUE/FALSE
**
**  CHANGED
**      15-Sep-97   floh    created
**      26-Jan-98   floh    + umbenannt, da nicht mehr Netzwerk-
**                            Level-spezifisch, und einen anderen
**                            MapParser eingebaut, der nicht mehr
**                            die ganze Map, sondern nur deren
**                            Größe liest.
**                          + jetzt mit PARSEMODE_NOINCLUDE, weil das
**                            (unnötige) Scannen der Includes Urzeiten
**                            gedauert hat...
*/
{
    BOOL result;
    struct ScriptParser parsers[3];
    memset(parsers, 0, sizeof(parsers));
    memset(ld, 0, sizeof(struct LevelDesc));

    parsers[0].parse_func = yw_LevelDataParser;
    parsers[0].target     = ld;
    parsers[0].store[0]   = (ULONG) ywd;

    parsers[1].parse_func = yw_LevelRoboParser;
    parsers[1].target     = ld;

    parsers[2].parse_func = yw_MapSizeOnlyParser;
    parsers[2].target     = ld;
    parsers[2].store[0]   = (ULONG) ywd;

    result = yw_ParseScript(script,3,parsers,
             (PARSEMODE_SLOPPY|PARSEMODE_NOINCLUDE));

    #ifdef YPA_DESIGNMODE
        strcpy(ywd->TypeMapName,(UBYTE *)&(ld->typ_map));
        strcpy(ywd->OwnerMapName,(UBYTE *)&(ld->own_map));
        strcpy(ywd->BuildMapName,(UBYTE *)&(ld->blg_map));
        strcpy(ywd->HeightMapName,(UBYTE *)&(ld->hgt_map));
    #endif

    return(result);
}

/*-----------------------------------------------------------------*/
BOOL yw_ParseLDFDebriefing(struct ypaworld_data *ywd,
                           struct LevelDesc *ld,
                           UBYTE *script)
/*
**  FUNCTION
**      LDF Parser für Debriefing, schreibt die VehicleProto-
**      Arrays bei Netzwerk-Levels nicht mehr um!
**
**  CHANGED
**      30-Sep-97   floh    created
**      26-Jan-98   floh    + PARSEMODE_NOINCLUDE
*/
{
    BOOL result;
    struct ScriptParser parsers[6];
    memset(parsers, 0, sizeof(parsers));
    memset(ld, 0, sizeof(struct LevelDesc));

    parsers[0].parse_func = yw_LevelDataParser;
    parsers[0].target     = ld;
    parsers[0].store[0]   = (ULONG) ywd;

    parsers[1].parse_func = yw_LevelGateParser;
    parsers[1].target     = ld;
    parsers[1].store[0]   = (ULONG) ywd;

    parsers[2].parse_func = yw_MissionMapParser;
    parsers[2].target     = ld;

    parsers[3].parse_func = yw_MapParser;
    parsers[3].target     = ld;
    parsers[3].store[0]   = (ULONG) ywd;

    parsers[4].parse_func = yw_DebriefingMapParser;
    parsers[4].target     = ld;

    parsers[5].parse_func = yw_LevelSuperitemParser;
    parsers[5].target     = ld;
    parsers[5].store[0]   = (ULONG) ywd;

    result = yw_ParseScript(script,6,parsers,
             (PARSEMODE_SLOPPY|PARSEMODE_NOINCLUDE));

    #ifdef YPA_DESIGNMODE
        strcpy(ywd->TypeMapName,(UBYTE *)&(ld->typ_map));
        strcpy(ywd->OwnerMapName,(UBYTE *)&(ld->own_map));
        strcpy(ywd->BuildMapName,(UBYTE *)&(ld->blg_map));
        strcpy(ywd->HeightMapName,(UBYTE *)&(ld->hgt_map));
    #endif

    return(result);
}


