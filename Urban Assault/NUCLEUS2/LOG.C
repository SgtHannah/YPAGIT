/*
**  $Source: PRG:VFM/Nucleus2/log.c,v $
**  $Revision: 38.4 $
**  $Date: 1998/01/06 12:43:40 $
**  $Locker: floh $
**  $Author: floh $
**
**  Simpler Log-Manager (war schon lange überflüssig)
**
**  (C) Copyright 1995 by A.Weissflog
*/
/*** Amiga Includes ***/
#include <exec/types.h>

/*** ANSI Includes ***/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

/*** Nucleus Includes ***/
#include "nucleus/nucleus2.h"
#include "nucleus/log.h"

_extern_use_nucleus

/****i* nucleus/_InitLog *********************************************
*
*   NAME   
*       _InitLog - initialisiert das Log-Msg-Subsystem
*
*   SYNOPSIS
*       success = nc_InitLog(channel)
*
*       BOOL nc_InitLog(UBYTE *)
*
*   FUNCTION
*       Initialisiert das Nucleus Log-Subsystem, mit dem 
*       Notfall-Messages an den User gerichtet werden können.
*       Normalerweise werden die Messages intern gepuffert
*       (bis zu NUM_LOGENTRIES Stück), um mit _CloseNucleus()
*       nach stdout geflusht zu werden. Grund: Manche Computersysteme
*       sind nicht in der Lage, gleichzeitig Gfx und ASCII-Ausgaben
*       hinzukriegen, deshalb wir gewartet, bis die Gfx Engine
*       geschlossen worden ist (das ist bei _CloseNucleus() normalerweise
*       der Fall.
*
*       Mittels _SetLogMode() kann man auch einen Immediate-Modus
*       einstellen, der versucht, die Message sofort zu dumpen,
*       allerdings kann das etwas unschön aussehen...
*
*       Nichtsdestotrotz :-) kann man das LogSystem auf
*       LOG_IMMEDIATE stellen, solange die Gfx-Engine noch
*       nicht initialisiert wurde.
*
*   INPUTS
*       channel - Optionaler Filename, nach dem die Ausgabe umgeleitet
*                 wird. Noch nicht implementiert, bis jetzt geht alles
*                 auf stdout raus.
*
*   RESULT
*       TRUE/FALSE, je nach Erfolg (zur Zeit immer TRUE)
*
*   SEE ALSO
*       nc_KillLog(), _SetLogMode(), _LogMsg()
*
*   HISTORY
*       04-Nov-95   floh    created
*       13-Apr-97   floh    + Output in File jetzt möglich
*
*****************************************************************************
*
*/
#ifdef AMIGA
BOOL __asm nc_InitLog(__a0 UBYTE *channel)
#else
BOOL nc_InitLog(UBYTE *channel)
#endif
{
    FILE *fp;
    fp = fopen("env/ypa_log.txt","w");
    if (fp) {
        fprintf(fp,"YPA General Log\n");
        fprintf(fp,"---------------\n");
        fclose(fp);
        return(TRUE);
    };
    return(FALSE);
}

/****i* nucleus/_KillLog *********************************************
*
*   NAME   
*       nc_KillLog - killt Log-Subsystem.
*
*   SYNOPSIS
*       void nc_KillLog(void)
*
*   FUNCTION
*       Killt das Log-Subsystem, alle noch ausstehenden Messages
*       werden in der Reihenfolge ihres Eintreffens geflusht.
*
*   INPUTS
*       ---
*
*   RESULT
*       ---
*
*   HISTORY
*       04-Nov-95   floh    created
*
*****************************************************************************
*
*/
void nc_KillLog(void)
{ }

/****** nucleus/_SetLogMode ****************************************
*
*   NAME   
*       _SetLogMode - setzt den aktuell gültigen Log-Modus.
*
*   SYNOPSIS
*       void _SetLogMode(mode)
*
*       void _SetLogMode(ULONG)
*
*   FUNCTION
*       Setzt den aktuell gültigen Log-Modus, der beeinflußt,
*       wie sich _LogMsg() verhält. Im Falle von LOGMODE_PUFFERED
*       (Default) werden alle Messages in einen internen
*       Puffer geschrieben, der 64 Einträge zu je 80 Byte
*       faßt (FIFO). Bei LOGMODE_IMMEDIATE werden die
*       Messages sofort per [sf]printf() rausgehauen,
*       egal was gerade läuft :-)
*
*   INPUTS
*       mode    - LOGMODE_PUFFERED|LOGMODE_IMMEDIATE
*
*   RESULT
*       ---
*
*   SEE ALSO
*       _LogMsg()
*
*   HISTORY
*       04-Nov-95   floh    created
*
*****************************************************************************
*
*/
#ifdef AMIGA
void __asm nc_SetLogMode(__d0 ULONG mode)
#else
void nc_SetLogMode(ULONG mode)
#endif
{ }

/****** nucleus/_LogMsg ********************************************
*
*   NAME   
*       _LogMsg - printf() like Message-Funktion
*
*   SYNOPSIS
*       void _LogMsg(string, [args], ...)
*
*       void _LogMsg(STRPTR, ...)
*
*   FUNCTION
*       Präsentiert dem User eine (Notfall-) Message, z.B.
*       Abbruch-Gründe, allgemeine Fehler, zur Not auch
*       Debugging-Messages. Die Message wird abhängig
*       vom aktuellen Log-Modus intern gepuffert und erst
*       mit _CloseNucleus() nach stdio geschrieben, oder
*       direkt ausgegeben.
*
*   INPUTS
*       string  - ein String-Pointer auf einen C-String, mit
*                 printf() Format-Codes falls gewünscht
*       args    - beliebig viele Args für die Format-Codes
*
*   RESULT
*       ---
*
*   EXAMPLE
*       _LogMsg("Mem allocation failed of size %d", mem_size);
*
*   NOTES
*       1. Der resultierende String darf 80 Zeichen nicht 
*          überschreiten
*       2. Der String sollte mit einem \n abgeschlossen sein.
*
*   HISTORY
*       04-Nov-95   floh    created
*       22-Nov-95   floh    oops, va_end() vergessen...
*       23-Mar-98   floh    + schreibt jetzt unmittelbar raus.
*
*****************************************************************************
*
*/
void nc_LogMsg(STRPTR string, ...)
{
    FILE *fp = fopen("env/ypa_log.txt","a");
    if (fp) {
        va_list arglist;
        va_start(arglist, string);
        vfprintf(fp,string,arglist);
        fclose(fp);
        va_end(arglist);
    };
}

/*-- EOF ----------------------------------------------------------*/

