/*
**  $Source: PRG:VFM/Nucleus2/config.c,v $
**  $Revision: 38.9 $
**  $Date: 1998/01/06 12:43:22 $
**  $Locker:  $
**  $Author: floh $
**
**  Nucleus2-Config-Handling.
**
**  (C) Copyright 1995 by A.Weissflog
*/
/*** Amiga Includes ***/
#include <exec/types.h>

/*** ANSI Includes ***/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/*** Nucleus Includes ***/
#include "nucleus/nucleus2.h"
#include "nucleus/config.h"

extern struct NucleusBase NBase;

/****** nucleus/_GetConfigItems ************************************
*
*   NAME   
*       _GetConfigItems - liest ConfigItems aus globalem 
*                         Config-Script
*
*   SYNOPSIS
*       success = _GetConfigItems(filename, item_array, num_items)
*
*       BOOL _GetConfigItems(STRPTR, struct ConfigItem *, ULONG)
*
*   FUNCTION
*       Öffnet einen beliebigen ASCII-Config-File bzw. den
*       plattformspezifischen Standard-Config-File und ermittelt
*       den Status eines oder mehrerer Config-Items.
*
*       Falls die Config-Items nicht im File definiert sind,
*       werden die Werte im <item_array> nicht verändert,
*       es ist also sinnvoll, diese mit Default-Werten zu
*       initialisieren.
*
*       Jedes Element im <item_array> muß folgendermaßen
*       initialisiert werden:
*
*           item.keyword    - String-Pointer auf Item-Keyword,
*                             z.B. "gfx.mode"
*           item.type       - Datentyp des ConfigItems, derzeit
*                             werden ausgewertet:
*                              CONFIG_INTEGER -> data = <LONG>
*                              CONFIG_STRING  -> data = <BOOL>
*                              CONFIG_BOOL    -> data = <char *>
*           item.data       - Hierhin wird der Status des Keywords
*                             im Config-File geschrieben. Es ist
*                             sinnvoll, dieses Feld mit einem
*                             Default-Wert zu initialisieren
*
*      Sonderfall item.type = CONFIG_STRING:
*      ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*          In diesem Fall ist <item.data> ein (char *) auf einen
*          extern bereitgestellten Buffer mit Mindestgröße
*          CONFIG_MAX_STRING_LEN (def. in "nucleus/config.h").
*          Der Buffer sollte mit einem Default-String initialisiert
*          sein. Falls das entsprechende ConfigItem im ConfigFile
*          gefunden wird, wird der zugehörige String einfach nach
*          ((char *) item.data) kopiert.
*
*
*       Die Namen der Standard-Config-Files sind:
*
*           AMIGA: env:nucleus.prefs
*           MSDOS: nucleus.cfg (im CurDir)
*           Win:   nucleus.ini (im CurDir)
*
*   INPUTS
*       filename    - Pfadname des Config-Files, wenn
*                     (filename == NULL) wird der Standard-
*                     Config-File verwendet
*       item_array  - Pointer auf ein Array von vorinitialisierten
*                     ConfigItem-Strukturen, deren Status aus
*                     dem Config-File gelesen werden soll.
*       num_items   - Anzahl Items im <item_array>
*
*   RESULT
*       success     - TRUE:  alles OK
*                     FALSE: konnte Config-File nicht öffnen, etc.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       nucleus/config.h
*
*   HISTORY
*       01-Feb-95   floh    created
*       02-Feb-95   floh    debugged & enhanced
*       07-Mar-96   floh    neuer Datentyp: CONFIG_ROL (Rest Of Line)
*       13-Apr-97   floh    bezieht jetzt das Cfg-Override-Array in
*                           den Test mit ein.
*
*****************************************************************************
*
*/
#ifdef AMIGA
BOOL __asm nc_GetConfigItems(__a0 STRPTR filename,
                             __a1 struct ConfigItem *ci_array,
                             __d0 ULONG ci_num)
#else
BOOL nc_GetConfigItems(STRPTR filename,
                       struct ConfigItem *ci_array,
                       ULONG ci_num)
#endif
{
    FILE *file;
    BOOL retval = FALSE;

    /* Config-File-Pfadname checken (Fallback auf Standard-Config-File) */
    if (filename == NULL) filename = CONFIG_FILE;

    /* öffne Standard-Config-File */
    if (file = fopen(filename,"r")) {

        UBYTE act_line[256];
        UBYTE *line_ptr;
        ULONG act_cfgindex = 0;

        /* lese jeweils eine Zeile ein, bis zum Ende... */
        while ((line_ptr = fgets(act_line, sizeof(act_line), file)) ||
               (act_cfgindex < NBase.CfgIndex))
        {
            UBYTE *dike_out, *keyword, *data;

            /*** Zeile aus File, oder aus Override-Items-Array? ***/
            if (!line_ptr) {
                /*** aus Override-Item... ***/
                strcpy(act_line,NBase.CfgItem[act_cfgindex++]);
            };

            /*** Zeile grob "vorparsen" ***/
            /* Kommentare + NewLine disablen */
            if (dike_out = strpbrk(act_line,";\n")) *dike_out = 0;

            /* erster Token -> Keyword */
            keyword = strtok(act_line, "= \t");
            if (keyword) {

                UWORD i;

                /* ok, keyword existiert, aber ist es ein gesuchtes? */
                for (i=0; i<ci_num; i++) {
                    if (stricmp(ci_array[i].keyword,keyword) == 0) {

                        /*** Volltreffer! ***/
                        switch(ci_array[i].type) {

                            /*-- INTEGER ----------------------------*/
                            case CONFIG_INTEGER:
                                data = strtok(NULL, "= \t");
                                if (data) {
                                    ci_array[i].data = strtol(data,&dike_out,0);
                                };
                                break;

                            /*--- BOOL ------------------------------*/
                            case CONFIG_BOOL:
                                data = strtok(NULL, "= \t");
                                if (data) {
                                    if ((stricmp("yes",data)  == 0) ||
                                        (stricmp("true",data) == 0) ||
                                        (stricmp("on",data)   == 0))
                                    {
                                        ci_array[i].data = (LONG) TRUE;
                                    } else {
                                        ci_array[i].data = (LONG) FALSE;
                                    };
                                };
                                break;

                            /*--- STRING ----------------------------*/
                            case CONFIG_STRING:
                                data = strtok(NULL, "= \t");
                                if (data) {
                                    strncpy((char *)ci_array[i].data, data,
                                            CONFIG_MAX_STRING_LEN);
                                };
                                break;

                            /*--- REST OF LINE ----------------------*/
                            case CONFIG_ROL:
                                /*** Anfang der restlichen Zeile ***/
                                data = strtok(NULL, "=");
                                if (data) {
                                    strncpy((char *)ci_array[i].data, data,
                                            CONFIG_MAX_STRING_LEN);
                                };
                                break;
                        }; // switch
                    };
                }; // for (i=0; i<ci_num; i++)
            }; // if (keyword)
        }; // while (fgets())

        fclose(file);
        retval = TRUE;

    }; /* if (file=fopen()) */

    /* Ende */
    return(retval);
}

/****** nucleus/_ConfigOverride *********************************************
*
*   NAME
*       _ConfigOverride - hiermit lassen sich beliebige Config-Items
*                         "vorgeben", ein entsprechender Eintrag
*                         im Config-File wird dann ignoriert.
*
*   SYNOPSIS
*       success = _ConfigOverride(STRPTR cfg_item);
*
*       BOOL _ConfigOverride(STRPTR cfg_item);
*
*   FUNCTION
*
*   RESULT
*       success     - TRUE:  alles OK
*                     FALSE: keine weiteren Config-Overrides mehr
*                            möglich oder cfg_item String ungültig
*
*   EXAMPLE
*
*   NOTES
*       Der übergebene String muß "static" sein.
*
*   BUGS
*
*   SEE ALSO
*       nucleus/config.h
*
*   HISTORY
*       13-Apr-97   floh    created
*
*****************************************************************************
*
*/
#ifdef AMIGA
BOOL __asm nc_ConfigOverride(__a0 STRPTR cfg_item)
#else
BOOL nc_ConfigOverride(STRPTR cfg_item)
#endif
{
    if (NBase.CfgIndex < (NC_NUM_CFG_OVERRIDE-1)) {
        NBase.CfgItem[NBase.CfgIndex++] = cfg_item;
        return(TRUE);
    };
    return(FALSE);
}

