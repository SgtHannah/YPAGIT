/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_winlocale.c,v $
**  $Revision: 38.2 $
**  $Date: 1998/01/06 16:30:26 $
**  $Locker: floh $
**  $Author: floh $
**
**  yw_winlocale.c -- Locale-Handler für Windows-Resource-DLLs.
**
**  (C) Copyright 1997 by A.Weissflog
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <direct.h>
#include <windows.h>
#include <windowsx.h>

extern HWND win_HWnd;
extern HINSTANCE win_LangDllInst;

extern void nc_LogMsg(char *string, ...);

/*-----------------------------------------------------------------*/
char *yw_LocStrCpyDll(char *to, char *from)
/*
**  FUNCTION
**      Filter-Kopierer für Strings aus Stringtable-Resourcen.
**      Macht folgendes:
**          -> konv. Klein- nach Großbuchstaben
**          -> wandelt ungültige Buchstaben in ein '*'
**          -> wandelt '_' nach ' '
**          -> erhält Newlines
**          -> erhält einen Buchstaben nach einer Formatierungs-
**             Anweisung (%d).
**
**  CHANGED
**      11-Sep-97   floh    created
**      13-Sep-97   floh    Backslashes werden in Multiline umgewandelt
**      02-Dec-97   floh    umgeschrieben auf fuer DBCS CharNext...
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
void yw_UnloadLocaleDll(void)
/*
**  CHANGED
**      19-Mar-98   floh    created
*/
{
    if (win_LangDllInst) {
        FreeLibrary(win_LangDllInst);
        win_LangDllInst = NULL;
    };
}

/*-----------------------------------------------------------------*/
unsigned long yw_LoadLocaleDll(char *lang,
                               char *buf,
                               char *buf_end,
                               char **lut,
                               unsigned long num_ids)
/*
**  FUNCTION
**      Versucht die DLL "levels/locale/[lang].dll" zu laden
**      und liest die eingebettete String-Resource.
**
**  INPUTS
**      lang    - kompletter Filename der Locale-DLL.
**      buf     - String-Puffer, hier werden alle String reinkopiert
**      buf_end - Ende+1 des String-Buffers
**      lut     - Pointer auf Lookup-Table für String-IDs.
**      num_ids - maximale Anzahl String-IDs, die übernommen werden sollen.
**
**  RESULTS
**      char * - returniert den aktuellen Pointer in den
**               String-Puffer.
**
**  CHANGED
**      11-Sep-97   floh    created
**      19-Mar-98   floh    + die LanguageDLL wird jetzt nicht mehr
**                            sofort wieder unloaded, weil die
**                            DlgBox-Resource für den Textinput jetzt
**                            in der Locale-DLL liegt,
**      31-Mar-98   floh    + <lang> ist jetzt der komplette Filename
*/
{
    BOOL retval = FALSE;
    BOOL result;

    /*** Filename zusammenbasteln und Library laden ***/
    yw_UnloadLocaleDll();
    win_LangDllInst = LoadLibraryEx(lang,NULL,DONT_RESOLVE_DLL_REFERENCES|LOAD_LIBRARY_AS_DATAFILE);
    if (win_LangDllInst) {
        unsigned long id;
        char temp_buf[4096];
        for (id=0; id<num_ids; id++) {
            unsigned long num_chrs;
            num_chrs = LoadString(win_LangDllInst,id,temp_buf,sizeof(temp_buf));
            if (num_chrs > 0) {
                unsigned long len = strlen(temp_buf)+1;
                if ((buf+len) < buf_end) {
                    yw_LocStrCpyDll(buf,temp_buf);
                    lut[id] = buf;
                    buf += len;
                };
            };
        };
        retval = TRUE;
    };
    return(retval);
}

/*-----------------------------------------------------------------*/
void yw_LaunchOnlineHelp(char *url)
/*
**  FUNCTION
**      Minimiert Main-Win und ruft Online-Hilfe auf.
**
**  CHANGED
**      05-Mar-98   floh    created
*/
{
    if (win_HWnd) {
        HINSTANCE inst;
        ShowWindow(win_HWnd,SW_MINIMIZE);
        inst = ShellExecute(win_HWnd,NULL,url,NULL,NULL,SW_RESTORE);
    };
}

/*-----------------------------------------------------------------*/
unsigned long yw_ReadRegistryKeyString(char *name, char *buf, long size_of_buf)
/*
**  FUNCTION
**      Liest einen Registry-Key als String und schreibt den 
**      Inhalt nach buf zurueck. <name> ist relativ zu
**      \\HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Microsoft Games\Urban Assault\1.0\
**
**  INPUTS
**      name    -> Name des Registry-Keys
**      buf     -> Pointer auf Puffer, in den zurueckgeschrieben wird
**      size_of_buf -> Groesse des Puffers in Byte
**
**  CHANGED
**      28-Apr-98   floh    created
*/
{
    char *key = "Software\\Microsoft\\Microsoft Games\\Urban Assault\\1.0";
    unsigned long retval = FALSE;
    char tmp_buf[MAX_PATH];
    HKEY hkey;
    
    memset(buf,0,size_of_buf);    
   	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, key, NULL, KEY_QUERY_VALUE, &hkey)) {
        DWORD max_len = MAX_PATH;
        DWORD type;
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, name, NULL, &type, tmp_buf, &max_len)) {
            if (type == REG_SZ) {
                strncpy(buf,tmp_buf,size_of_buf-1);
                retval = TRUE;
            };
        };
        RegCloseKey(hkey);
    };
    return(retval);
}      

/*-----------------------------------------------------------------*/
void yw_WinMessageBox(char *title, char *text)
/*
**  FUNCTION
**      Zeigt eine Standard-Messagebox an. Ausserhalb muss
**      EnableGDI/DisableGDI erledigt werden.
**
**  CHANGED
**      23-May-98   floh    Standard-Funktions-Header rangehaengt.
*/
{
    MessageBox(win_HWnd,text,title,0);
}

/*-----------------------------------------------------------------*/
unsigned long yw_RawCDCheck(unsigned long check_install_type)
/*
**  FUNCTION
**      Testet, ob die YPA-CD im Laufwerk liegt. Der 
**      Test passiert nur einmalig, irgendwelche
**      Retry-Messageboxen muessen ausserhalb erledigt
**      werden.
**      Sind die Registy-Keys nicht vorhanden, wird kein
**      Test vorgenommen. 
**
**      Registry-Keys sind folgendermassen:
**
**          drive_letter = "D:\"    
**          install_type = "Compact", "Typical", "Complete" 
**      
**      Als Volume-Name wird "UAssault" erwartet. 
**
**  CHANGED
**      23-May-98   floh    created
*/
{
    char drive_letter[32];
    char install_type[64];
    unsigned long retval = TRUE;
    
    nc_LogMsg("-> yw_CDCheck() entered.\n");
    if (yw_ReadRegistryKeyString("InstalledFrom",drive_letter,sizeof(drive_letter))) {
        if (yw_ReadRegistryKeyString("InstalledGroup",install_type,sizeof(install_type))) {
        
            char vol_name[256];
            DWORD vol_ser_num;
            DWORD max_comp_len;
            DWORD fs_flags;
            char fs_name[256];
            BOOL res; 
            DWORD error;           
            
            nc_LogMsg("-> RegKeys read (InstalledFrom = %s, InstalledGroup = %s)\n",drive_letter, install_type);             
            
            /*** Full Install und <check_install_type>? ***/
            if (check_install_type && (stricmp("7",install_type)==0)) return(TRUE);
            
            /*** ansonsten checken, ob die richtige CD im Laufwerk liegt ***/
            SetErrorMode(SEM_FAILCRITICALERRORS);
            res = GetVolumeInformation(drive_letter, vol_name, sizeof(vol_name),
                                       &vol_ser_num, &max_comp_len, &fs_flags,
                                       &fs_name, sizeof(fs_name));
            if (!res) error = GetLastError();
            SetErrorMode(0); 
            if (res) {
                nc_LogMsg("-> GetVolumeInformation() succeeded.\n");
                nc_LogMsg("-> vol_name     = %s\n",vol_name);
                nc_LogMsg("-> vol_ser_num  = %d\n",vol_ser_num);
                nc_LogMsg("-> max_comp_len = %d\n",max_comp_len);
                nc_LogMsg("-> fs_name      = %s\n",fs_name);
                if (stricmp(vol_name,"UAssault")!=0) retval = FALSE;
            } else {
                nc_LogMsg("-> GetVolumeInformation() failed because %d.\n",error);
                retval = FALSE;
            };
        };
    };
    return(retval);
}

/*-----------------------------------------------------------------*/
unsigned long yw_RetryCancelMessageBox(char *title_text, char *body_text)
/*
**  FUNCTION
**      Gibt eine OK/Cancel-Messagebox aus, EnableGDI/DisableGDI
**      muss ausserhalb erledigt werden!
**
**  CHANGED
**      24-May-98   floh    created
*/
{
    int result;
    result = MessageBox(win_HWnd,body_text,title_text,MB_OKCANCEL|MB_ICONWARNING|MB_APPLMODAL);
    return((result == IDCANCEL) ? FALSE:TRUE);
}
 
 


    
            
            


            
                
                
        
    

