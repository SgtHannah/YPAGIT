/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_locale.c,v $
**  $Revision: 38.4 $
**  $Date: 1998/01/06 16:22:54 $
**  $Locker: floh $
**  $Author: floh $
**
**  YPA-Locale-System.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "nucleus/nucleus2.h"
#include "ypa/ypaworldclass.h"
#include "ypa/ypalocale.h"

#include "yw_protos.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus

/*-------------------------------------------------------------------
**  PLEASE NOTE!
**  ~~~~~~~~~~~~
**  Die wichtigste Funktion des Locale-Systems, ypa_GetStr()
**  ist keine Funktion, sondern ein Macro und definiert
**  in "ypa/ypalocale.h".
*/

/*-----------------------------------------------------------------*/
void yw_LocaleReset(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Trifft alle Vorbereitungen für ein neues Locale-Set,
**      welches durch wiederholten Aufruf von
**      yw_LocaleAddStr() erzeugt wird.
**
**  CHANGED
**      24-Jul-96   floh    created
**      31-Jul-96   floh    setzt ywd->LocaleLang auf default
*/
{
    ywd->LocalePtr = ywd->LocaleStrBuf;
    ywd->LocaleEOB = ywd->LocaleStrBuf + LOCALE_SIZEOF_STRBUF;
    memset(ywd->LocaleStrBuf,0,LOCALE_SIZEOF_STRBUF);
    memset(ywd->LocaleArray,0,LOCALE_MAX_STRINGS*sizeof(UBYTE *));
    memset(ywd->LocaleLang,0,sizeof(ywd->LocaleLang));
    strcpy(ywd->LocaleLang,"default");
}

/*-----------------------------------------------------------------*/
UBYTE *yw_LocStrCpy(UBYTE *to, UBYTE *from)
/*
**  FUNCTION
**      Smarte Stringcopy-Funktion, filtert den zu
**      kopierenden String folgendermaßen:
**
**          Unterstrich     -> Leerzeichen
**          Kleinbuchstaben -> Großbuchstaben
**          ungültig        -> *
**
**  CHANGED
**      16-Oct-96   floh    created
**      13-Aug-97   floh    Newline wird nicht mehr gefiltert
**      11-Sep-97   floh    + vereinheitlicht, erhält jetzt Formatierungs-
**                            Anweisungen (also %d mäßiges Zeug)
**      18-Sep-97   floh    + Filter für deutsche extended Chars
**      02-Dec-97   floh    + umgeschrieben auf DBCS
*/
{
    #ifdef __DBCS__
        char c;
        /*** String kopieren und Newlines reinschreiben ***/
        strcpy(to,from);
        while (c = *to) {
            if ('\\'==c) *to='\n';
            to = dbcs_NextChar(to);
        };
        to++;
    #else
        char c;
        unsigned long prev_format = FALSE;
        while (c = *from) {
            if (!prev_format) c=toupper(c);
            if (c=='_') c=' ';
            else if (c == '\\') c = '\n';
            else if ((c=='ä')||(c=='Ä')) c=192;
            else if ((c=='ö')||(c=='Ö')) c=193;
            else if ((c=='ü')||(c=='Ü')) c=194;
            else if (c=='ß') c=195;
            else if (((c<' ')||(c>='a')) && (c!='\n')) c='*';
            if (c == '%') prev_format = TRUE;
            else          prev_format = FALSE;
            *to++ = c;
        };
        *to++ = 0;
    };
    #endif
    return(to);
}

/*-----------------------------------------------------------------*/
ULONG yw_LocaleAddStr(struct ypaworld_data *ywd, ULONG id, UBYTE *str)
/*
**  FUNCTION
**      Weist dem String <str> die Locale-ID <id> zu.
**      Der String wird kopiert, dabei werden Kleinbuchstaben
**      in Großbuchstaben umgewandelt und ungültige Zeichen
**      als '*' geschrieben, '_' werden in Space umgewandelt.
**
**  INPUTS
**      ywd     - Ptr auf LID des Weltobjects
**      id      - Locale-ID des Strings (z.B. STR_YES)
**      str     - der String itself
**
**  RESULT
**      0   -> alles OK
**      1   -> <id> ungültig
**      2   -> StrBuf ist voll
**      3   -> String war bereits belegt
**
**  CHANGED
**      24-Jul-96   floh    created
**      31-Jul-96   floh    der 1.Buchstabe nach einem '%' wird
**                          nicht toupper()'d, um printf()-like
**                          Format-Strings einigermaßen zu unterstützen
**      13-Aug-97   floh    + NewLine wird nicht mehr gefiltert
*/
{
    UBYTE *to  = ywd->LocalePtr;
    UBYTE *end = ywd->LocaleEOB;

    if (id >= LOCALE_MAX_STRINGS) return(1); // <id> ungültig
    if (to >= end)                return(2); // StrBuf ist voll
    if (ywd->LocaleArray[id])     return(3); // String bereits belegt

    ywd->LocaleArray[id] = to;
    ywd->LocalePtr = yw_LocStrCpy(to,str);
    return(0);
}

/*-----------------------------------------------------------------*/
BOOL yw_InitLocale(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Initialisiert Locale-System. Aufruf: einmalig
**      aus ypaworld.class/OM_NEW.
**
**  CHANGED
**      24-Jul-96   floh    created
**      28-Jul-96   floh    ywd->LocHandle wird ausgefüllt
*/
{
    BOOL retval = FALSE;

    ywd->LocaleStrBuf = (UBYTE *) _AllocVec(LOCALE_SIZEOF_STRBUF, MEMF_PUBLIC);
    if (ywd->LocaleStrBuf) {
        ywd->LocaleArray = (UBYTE **)
            _AllocVec(LOCALE_MAX_STRINGS*sizeof(UBYTE *),MEMF_PUBLIC|MEMF_CLEAR);
        if (ywd->LocaleArray) {
            ywd->LocHandle = ywd->LocaleArray;
            yw_LocaleReset(ywd);
            retval = TRUE;
        };
    };
    if (!retval) yw_KillLocale(ywd);
    return(retval);
}

/*-----------------------------------------------------------------*/
void yw_KillLocale(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Gibt alle Locale-Resourcen frei.
**
**  CHANGED
**      24-Jul-96   floh    created
**      19-Mar-98   floh    + falls noch geladen, wird die Locale-DLL
**                            gekillt
*/
{
    yw_UnloadLocaleDll();
    if (ywd->LocaleStrBuf) _FreeVec(ywd->LocaleStrBuf);
    if (ywd->LocaleArray)  _FreeVec(ywd->LocaleArray);
    ywd->LocaleStrBuf = NULL;
    ywd->LocalePtr    = NULL;
    ywd->LocaleEOB    = NULL;
    ywd->LocaleArray  = NULL;
}

/*-----------------------------------------------------------------*/
ULONG yw_LocaleScriptParser(struct ScriptParser *p)
/*
**  FUNCTION
**      Kern des Locale-Parsers, akzeptiert Zeilen der
**      Form
**
**          16 = Das_ist_ein_String
**
**      16 ist die numerische Locale-ID (siehe "ypa/ypalocale.h"),
**      die Leerzeichen im String müssen durch Unterstrich
**      ersetzt werden.
**
**      Kleinbuchstaben werden zwar in Großbuchstaben umgewandelt,
**      ich schlage aber vor, trotzdem die korrekte Groß/Klein-
**      schreibung zu verwenden, man weiß ja nie...
**
**  CHANGED
**      24-Jul-96   floh    created
*/
{
    struct ypaworld_data *ywd = (struct ypaworld_data *) p->target;
    UBYTE *kw   = p->keyword;
    UBYTE *data = p->data;

    if (PARSESTAT_READY == p->status) {

        /*** momentan außerhalb eines Contexts ***/
        if (stricmp(kw, "begin_locale")==0) {
            p->status = PARSESTAT_RUNNING;
            return(PARSE_ENTERED_CONTEXT);
        } else return(PARSE_UNKNOWN_KEYWORD);

    } else {

        ULONG id,res;

        /*** momentan innerhalb eines Context ***/
        if (stricmp(kw,"end")==0) {
            /*** ...und raus. ***/
            p->status = PARSESTAT_READY;
            return(PARSE_LEFT_CONTEXT);
        };

        /*** aktuelles Keyword ist Locale-ID ***/
        id  = strtol(kw,NULL,0);
        res = yw_LocaleAddStr(ywd, id, data);
        switch(res) {
            case 1:
                _LogMsg("Locale parser: id [%d] too big.\n",id);
                return(PARSE_BOGUS_DATA);
            case 2:
                _LogMsg("Locale parser: buffer overflow at id [%d].\n",id);
                return(PARSE_BOGUS_DATA);
            case 3:
                _LogMsg("Locale parser: id [%d] already defined.\n",id);
                return(PARSE_BOGUS_DATA);
        };

        return(PARSE_ALL_OK);
    };

    /*** can't happen ***/
    return(PARSE_UNKNOWN_KEYWORD);
}

/*-----------------------------------------------------------------*/
BOOL yw_LoadLocaleLng(struct ypaworld_data *ywd, UBYTE *lang)
/*
**  FUNCTION
**      Lädt die Locale-Strings aus einem #?.lng
**      File namens "levels/locale/[lang].lng" zu
**      laden.
**
**  CHANGED
**      11-Sep-97   floh    created
*/
{
    struct ScriptParser p[1];
    UBYTE fname[128];
    BOOL retval = FALSE;

    /*** Parse-Parameter initialisieren ***/
    sprintf(fname, "data:locale/%s.lng", lang);
    memset(p,0,sizeof(p));
    p[0].parse_func = yw_LocaleScriptParser;
    p[0].target     = ywd;
    if (yw_ParseScript(fname,1,p,PARSEMODE_EXACT)) {
        retval = TRUE;
    } else {
        _LogMsg("ERROR: Could not load language file '%s'!!!\n",fname);
    };
    return(retval);
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, yw_YWM_SETLANGUAGE, struct setlanguage_msg *msg)
/*
**  CHANGED
**      24-Jul-96   floh    created
**      31-Jul-96   floh    kopiert msg->lang nach ywd->LocaleLang
*/
{
    struct ypaworld_data *ywd = INST_DATA(cl,o);
    BOOL dll_loaded = FALSE;
    UBYTE *use_system_text_input;
    UBYTE buf1[256];
    UBYTE buf2[256];

    /*** LocaleSystem zurücksetzen ***/
    yw_LocaleReset(ywd);
    strcpy(ywd->LocaleLang,msg->lang);

    /*** Windows: zuerst DLL versuchen ***/
    sprintf(buf1,"data:locale/%s.dll",ywd->LocaleLang);
    _ManglePath(buf1,buf2,sizeof(buf2));
    if (yw_LoadLocaleDll(buf2, ywd->LocaleStrBuf,
                         ywd->LocaleEOB, ywd->LocaleArray,
                         LOCALE_MAX_STRINGS))
    {
        dll_loaded = TRUE;
    };

    /*** sonst gucken, ob ein konventioneller LNG-File da ist ***/
    if (!dll_loaded) {
        if (!yw_LoadLocaleLng(ywd,ywd->LocaleLang)) {
            yw_LocaleReset(ywd);
            return(FALSE);
        };
    };

    /*** falls DBCS, TrueType Font einstellen ***/
    #ifdef __DBCS__
    if (ywd->DspXRes < 512) {
        dbcs_SetFont(ypa_GetStr(ywd->LocHandle,STR_FONTDEFINE_LRES,"Arial,8,400,0"));
    } else {
        dbcs_SetFont(ypa_GetStr(ywd->LocHandle,STR_FONTDEFINE,"MS Sans Serif,12,400,0"));
    };
    #endif

    /*** UseSystemInput Flag gesetzt? ***/
    use_system_text_input = ypa_GetStr(ywd->LocHandle,STR_USESYSTEMTEXTINPUT,"FALSE");
    if (stricmp(use_system_text_input,"FALSE")==0) ywd->UseSystemTextInput = FALSE;
    else                                           ywd->UseSystemTextInput = TRUE;


    /*** all ok ***/
    return(TRUE);
}

