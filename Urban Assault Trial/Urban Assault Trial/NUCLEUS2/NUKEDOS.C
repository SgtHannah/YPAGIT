/*
**  $Source: PRG:VFM/Nucleus2/nukedos.c,v $
**  $Revision: 38.9 $
**  $Date: 1998/01/06 12:44:05 $
**  $Locker:  $
**  $Author: floh $
**
**  Engines laden/killen, Diskobject-Stuff, alles
**  100% portabel, weil ausschließlich mit Nucleus-Kernel-Routinen
**  bzw. ANSI-C-Lib-Routinen gearbeitet wird.
**
**  (C) Copyright 1994 by A.Weissflog
*/
#include <exec/types.h>
#include <libraries/iffparse.h>

#include <string.h>

#include "modules.h"
#include "nucleus/nucleus2.h"
#include "nucleus/class.h"
#include "nucleus/nucleusclass.h"
#include "nucleus/nukedos.h"

/*-------------------------------------------------------------------
**  Externals
*/
extern struct NucleusBase NBase;
extern struct GET_Nucleus nuc_NUC_GET;

/****i* nucleus/_LoadInnerObject *************************************
*
*   NAME   
*       _LoadInnerObject - lade ein eingebettes Disk-Objekt
*
*   SYNOPSIS
*       object = _LoadInnerObject(iff)
*
*       Object *_LoadInnerObject(struct IFFHandle *)
*
*   FUNCTION
*       Erzeugt ein neues Objekt aus einem bereits offenen
*       IFF-Handle.
*       Verwendung:
*           1) Um weitere in einem OBJT-Chunk EINGEBETTETE
*              OBJT-Chunks zu lesen (von OM_NEWFROMIFF-Dispatcher
*              aufzurufen).
*           2) NewDiskObject() ruft ebenfalls NewEmbeddedObject()
*              auf.
*
*   INPUTS
*       iff     - Pointer auf offenes IFF-Handle, gestoppt auf 
*                 OBJT-Chunk
*
*   RESULT
*       object  - Handle des fertigen Objects, oder NULL bei 
*                 Mißerfolg
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       _load(), _save(), _SaveInnerObject()
*
*   HISTORY
*      16-Oct-93   floh    created
*      09-Nov-93   floh    Wenn ein FORM-Chunk (also ein Klassen-Chunk)
*                          verlassen wird, wurde noch das EndOfChunk
*                          geholt. Das dürfte definitiv ein Bug gewesen
*                          sein.
*      18-Dec-93   floh    CLID-Chunks mit der ClassID wurden bisher
*                          mit einem GetClassPtr(ClassID) in einen
*                          Pointer auf die Klassenstruktur gewandelt.
*                          Statt GetClassPtr() wird jetzt ein MakeClass()
*                          angewendet, damit automatisch die Klasse
*                          bereitsgestellt, deren Instanz das Object wird.
*                          Der Applikations-Programmierer braucht sich
*                          jetzt nicht mehr um die benötigten Klassen eines
*                          DiskObjects zu kümmern.
*      28-Mar-94   floh    Wenn das Erzeugen des Objects nicht geklappt
*                          hat, wird jetzt ein FreeClass() auf die
*                          Objekt-Klasse angewendet, siehe NewObject()
*                          in >nucleus.c< für nähere Erklärung.
*      01-Jun-94   floh    temporärer serieller Debug-Code.
*                          (wurde wieder entfernt)
*      06-Jun-94   floh    Besseres ANSI-C, PC-Version über bedingte
*                          Compilierung eingebunden.
*      07-Jun-94   floh    Der OM_NEWFROMIFF-Dispatcher wird jetzt
*                          nicht mehr über eine externe Support-Routine
*                          aufgerufen, sondern über einen indirekten
*                          Function-Call
*      31-Oct-94   floh    nc_Prefixe
*      28-Dec-94   floh    übernommen nach Nucleus2, leichte Modifikationen
*      22-Jan-96   floh    Bugfix: falls das Laden eines Objects nicht
*                          klappte, wurde nicht unmittelbar abgebrochen,
*                          sondern der File weitergescannt... :-/
*
*****************************************************************************
*
*/
#ifdef AMIGA
Object *__asm nc_LoadInnerObject(__a0 struct IFFHandle *iff)
#else
Object *nc_LoadInnerObject(struct IFFHandle *iff)
#endif
{
    /* Prototype für Methoden-Aufruf */
    #ifdef AMIGA
        ULONG __asm (*dispatcher) (__a0 Object *o, __a1 Class *cl, __a2 void *msg);
    #else
        ULONG (*dispatcher) (Object *o, Class *cl, void *msg);
    #endif

    /* Scanne File, als nächstes müßte CLID kommen, bei der */
    /* ersten FORM wird auf alle Fälle zum Methoden-Dispatcher verzweigt */

    LONG error;
    Object *o = NULL;
    Class *cl = NULL;
    struct ContextNode *cn;
    while ((error = nc_ParseIFF(iff, IFFPARSE_RAWSTEP)) != IFFERR_EOC) {
        /* andere Errors als EndOfChunk abfangen */
        if (error) return(NULL);

        /* Contextnode holen */
        cn = nc_CurrentChunk(iff);

        /* ClassID-Chunk? */
        if (cn->cn_ID == CHUNK_ClassID) {

            /* nullterminierte ClassID in eigenen Buffer lesen */
            UBYTE clid[256];
            if ((error = nc_ReadChunkBytes(iff, clid, sizeof(clid))) < 0) {
                return(NULL);
            };
            /* wenn erfolgreich, MakeClass(clid) ausführen */
            cl = nc_MakeClass(clid);
            if (!cl) return(NULL);

            /* End Of Chunk holen */
            nc_ParseIFF(iff,IFFPARSE_RAWSTEP);
            continue;
        };

        /* die eigentliche Object-FORM? */
        if (cn->cn_ID == ID_FORM) {
            Class *virtcl;
            struct iff_msg iffmsg;

            /* Message für OM_NEWFROMIFF-Dispatcher ausfüllen */
            iffmsg.iffhandle = iff;

            /* Dispatcher und "seine" Klasse laden */
            virtcl     = cl->methods[OM_NEWFROMIFF].trueclass;
            dispatcher = cl->methods[OM_NEWFROMIFF].dispatcher;

            /* OM_NEWFROMIFF-Dispatcher aufrufen */
            /* die TrueClass wird in <o> hochgegeben */
            o = (Object *) dispatcher((Object *)cl,virtcl,(void *)&iffmsg);

            if (!o) {
                nc_FreeClass(cl);
                return(NULL);
            };

            /* und das war auch schon der wichtigste Part */
            continue;
        };

        /* unbekannte Chunks überspringen */
        if (!nc_SkipChunk(iff)) return(NULL);
    };

    return(o);
}

/****** nucleus/_load **********************************************
*
*   NAME   
*       _load - erzeugt Object aus einem Disk-Object
*
*   SYNOPSIS
*       object = _load(filename)
*
*       Object *_load(STRPTR)
*
*   FUNCTION
*       Erzeugt ein "benutzbares" Nucleus-Objekt aus einem
*       (sogenannten "persistenten") Disk-Objekt.
*       Das Disk-Objekt muß irgendwann vorher mit _save()
*       erzeugt worden sein.
*
*       _load() öffnet nur den IFF-File, sucht den Class-ID
*       Chunk (eigentlich macht das alles schon _LoadInnerObject()),
*       initialisiert die Klasse und wendet dann OM_NEWFROMIFF
*       an. Das Objekt lädt sich quasi selbst.
*
*       Das ist auch der Grund, warum es kein eigenständiges
*       "Objekt-Fileformat" gibt, weil jede Klasse den
*       Code zum Speichern/Laden der eigenen Instanzen
*       selbst enthält. Neue Nucleus-Klassen erweitern
*       also das quasi Fileformat.
*
*   INPUTS
*       filename    - vollständiger Pfadname des zu ladenden
*                     Disk-Objekt-Files
*
*   RESULT
*       object  - Handle des fertigen Objects, oder NULL bei 
*                 Mißerfolg
*
*   EXAMPLE
*       Object *o;
*       if (o = _load("mc2resources:Objects/example_object")) {
*           _method(o, MTD_IRGENDWAS, irgendwie);
*           _dispose(o);
*       };
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       _save(), _LoadInnerObject()
*
*   HISTORY
*      16-Sep-93   floh    created
*      16-Oct-93   floh    Ruft jetzt einfach NewEmbeddedObject() auf,
*                          um OBJT-Chunk abzuhandeln.
*      06-Jun-94   floh    besseres ANSI-C, PC-Version über bedingte
*                          Compilierung eingebunden
*      31-Oct-94   floh    nc_Prefixe
*      28-Dec-94   floh    1) hierher übernommen aus Nucleus1
*                             (umbenannt und modifiziert).
*                          2) zweitens einen fetten Bug rausgenommen,
*                             der einen Non-Null-Object-Pointer
*                             zurückbrachte, auch wenn ein Fehler auftrat
*      01-Dec-94   floh    debugging... & debugging...
*
*****************************************************************************
*
*/
#ifdef AMIGA
Object *__asm nc_LoadObject(__a0 STRPTR filename)
#else
Object *nc_LoadObject(STRPTR filename)
#endif
{
    Object *o = NULL;
    LONG error;
    struct IFFHandle *iff;
    struct ContextNode *cn;

    /* gib mir einen IFFHandle... */
    iff = nc_AllocIFF();
    if (!iff) return(NULL);

    /* Diskobject-File öffnen */
    iff->iff_Stream = (ULONG) nc_FOpen(filename, "rb");
    if (!(iff->iff_Stream)) {
        nc_FreeIFF(iff); return(NULL);
    };

    /* IFF-Stream-Handler initialisieren */
    nc_InitIFFasNucleus(iff);

    /* IFF-Stream zum Lesen öffnen */
    error = nc_OpenIFF(iff, IFFF_READ);
    if (error != 0) {
        nc_FClose((APTR)iff->iff_Stream); nc_FreeIFF(iff); return(NULL);
    };

    /* MC2-File? */
    error = nc_ParseIFF(iff, IFFPARSE_RAWSTEP);
    if (error != 0) goto closeall;
    cn = nc_CurrentChunk(iff);
    if ((cn->cn_ID != ID_FORM) || (cn->cn_Type != FORM_MC2)) goto closeall;

    /* OBJT-File ? */
    error = nc_ParseIFF(iff, IFFPARSE_RAWSTEP);
    if (error != 0) goto closeall;
    cn = nc_CurrentChunk(iff);
    if ((cn->cn_ID != ID_FORM) || (cn->cn_Type != FORM_Object)) goto closeall;

    /* OBJT-Chunk lesen... */
    o = nc_LoadInnerObject(iff);

    /* beenden... */
closeall:
    nc_CloseIFF(iff);
    nc_FClose((APTR)iff->iff_Stream);
    nc_FreeIFF(iff);
    return(o);
}

/****i* nucleus/_SaveInnerObject *************************************
*
*   NAME   
*       _SaveInnerObject    - erzeugt ein eingebettetes Disk-Objekt
*
*   SYNOPSIS
*       success = _SaveInnerObject(object, iff)
*
*       BOOL _SaveInnerObject(Object *, struct IFFHandle *)
*
*   FUNCTION
*       Gibt einem konventionellen Nucleus-Objekt die Anweisung,
*       seinen Status in einen bereits geöffneten IFF-Handle
*       zu sichern.
*
*       Das Original-Objekt bleibt existent.
*
*   INPUTS
*       object  - ein existierendes Objekt
*       iff     - ein offener IFF-Handle
*
*   RESULT
*       success - TRUE:  alles OK,
*                 FALSE: Ooops, schwerwiegender Fehler, das erzeugte
*                        Diskobjekt ist nicht vollständig.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       _LoadInnerObject(), _save(), _SaveInnerObject()
*
*   HISTORY
*      16-Oct-93   floh    created
*      06-Jun-94   floh    Besseres ANSI-C, PC-Version über bedingte
*                          Compilierung eingebunden.
*      31-Oct-94   floh    nc_Prefixe
*      28-Dec-94   floh    übernommen aus Nucleus1 und leicht modifiziert
*
*****************************************************************************
*
*/
#ifdef AMIGA
BOOL __asm nc_SaveInnerObject(__a0 Object *o, __a1 struct IFFHandle *iff)
#else
BOOL nc_SaveInnerObject(Object *o, struct IFFHandle *iff)
#endif
{
    LONG error;
    STRPTR clid;
    ULONG clid_len;
    BOOL retvalue;

    /* FORM OBJT schreiben */
    error = nc_PushChunk(iff, FORM_Object, ID_FORM, IFFSIZE_UNKNOWN);
    if (error != 0) return(FALSE);

    /* Chunk CLID schreiben... */
    /* zuerst muß der Pointer auf den ClassID-String her, dabei */
    /* muß ich direkt auf die LID der Nucleus-Data zugreifen,   */
    /* was ansonsten natürlich strengstens verboten ist.        */
    /* (Don't try this at home kids, I'm a trained professional)*/

    clid = ((struct nucleusdata *)o)->o_Class->node.ln_Name;

    /* Länge der ClassID + 0-Terminator */
    clid_len = strlen(clid)+1;

    error = nc_PushChunk(iff, 0L, CHUNK_ClassID, clid_len);
    if (error != 0) return(FALSE);

    /* ClassID nach Chunk CLID schreiben */
    error = nc_WriteChunkBytes(iff, clid, clid_len);
    if (error < 0) return(FALSE);

    /* CLID-Chunk zumachen */
    nc_PopChunk(iff);

    /* Kontrolle an OM_SAVETOIFF-Dispatcher */
    retvalue = nc_DoMethod(o, OM_SAVETOIFF, (ULONG) iff);

    /* Reinitialisierung */
    nc_PopChunk(iff);  /* FORM OBJT */

    return(retvalue);
}

/****** nucleus/_save **********************************************
*
*   NAME   
*       _save - erzeugt ein Disk-Objekt
*
*   SYNOPSIS
*       success = _save(object, filename)
*
*       BOOL _save(Object *, STRPTR)
*
*   FUNCTION
*       Gibt einem Nucleus-Objekt die Anweisung, sich in ein
*       (per Filename definierten) Disk-Objekt zu sichern.
*
*       Das Original-Objekt bleibt existent.
*
*   INPUTS
*       object   - das zu savende Objekt
*       filename - vollständiger Pfadname des Disk-Objekts
*
*   RESULT
*       success  - TRUE:  alles OK
*                  FALSE: Ooops...
*
*   EXAMPLE
*       Object *o;
*       o = _new("yppsn.class", YPSA_YODEL, 1,
*                               YPSA_DODEL, 2,
*                               TAG_DONE);
*       if (o) {
*           _save(o, "mc2resources:Objects/yodel");
*           _dispose(o);
*       };
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       _SaveInnerObject(), _load(), _LoadInnerObject()
*
*   HISTORY
*      18-Sep-93   floh    created
*      06-Jun-94   floh    besseres ANSI-C, PC-Version über bedingte
*                          Compilierung eingebunden
*      31-Oct-94   floh    nc_Prefixe
*      28-Dec-94   floh    komplett neu implementiert, weil sowieso
*                          übernommen aus Nucleus1
*
*****************************************************************************
*
*/
#ifdef AMIGA
BOOL __asm nc_SaveObject(__a0 Object *o, __a1 STRPTR filename)
#else
BOOL nc_SaveObject(Object *o, STRPTR filename)
#endif
{
    BOOL retvalue = FALSE;
    struct IFFHandle *iff;

    if (iff = nc_AllocIFF()) {

        /* File erzeugen */
        if (iff->iff_Stream = (ULONG) nc_FOpen(filename, "wb")) {

            /* IFF-Stream-Handler einklinken */
            nc_InitIFFasNucleus(iff);

            /* IFF-Stream aufmachen */
            if (nc_OpenIFF(iff,IFFF_WRITE) == 0) {

                /* FORM MC2 schreiben */
                if (nc_PushChunk(iff,FORM_MC2,ID_FORM,IFFSIZE_UNKNOWN)==0) {

                    /* OBJT-Chunk erzeugen */
                    retvalue = nc_SaveInnerObject(o,iff);

                    /* FORM MC2 poppen */
                    nc_PopChunk(iff);
                };
                /* IFF-Stream zumachen */
                nc_CloseIFF(iff);
            };
            /* File schließen */
            nc_FClose((APTR)iff->iff_Stream);
        };
        /* IFFHandle freigeben */
        nc_FreeIFF(iff);
    };

    /* fertig */
    return(retvalue);
}

/****** nucleus/_OpenEngine ****************************************
*
*   NAME   
*       _OpenEngine - öffnet und initialisiert eine Engine 
*                     zur Benutzung
*
*   SYNOPSIS
*       entry = _OpenEngine(id)
*
*       struct GET_Engine *_OpenEngine(ULONG)
*
*   FUNCTION
*       Öffnet die durch <id> definierte Engine zur weiteren 
*       Benutzung.
*
*       Engines sind dynamisch ladbare Codesegmente (jedenfalls
*       auf Plattformen, die das DYNAMIC_LINKING unterstützen),
*       die als Hardware-Treiber benutzt werden können.
*       Die <id> beschreibt einen bestimmten Typ von Engine,
*       der jeweils ein typspezifisches Standard-API anbietet.
*       Das VFM-Klassenmodell macht massiven Gebrauch von
*       den Engines, es ist aber teilweise auch notwendig,
*       Engine-Routinen direkt zu benutzen.
*
*       Bevor Engine-Routinen benutzt werden sollen, bzw
*       Objekte erzeugt werden, deren Klassen-Segmente
*       auf die Engines zugreifen, ist es notwendig, die
*       Engines zu öffnen. Eben dies geschieht mit
*       _OpenEngine().
*
*   INPUTS
*       id  - eins von 
*                 MID_ENGINE_OUTPUT_VISUAL
*                 MID_ENGINE_INPUT
*                 MID_ENGINE_TRANSFORM,
*             definiert in "modules.h"
*
*   RESULT
*       entry   - Pointer auf Entry-Table der Engine, oder
*                 NULL bei Mißerfolg.
*
*   EXAMPLE
*       siehe einen der zahlreichen mitgelieferten Examples :-)
*
*   NOTES
*       Das Engine-Konzept entstammt noch aus den Urzeiten
*       von MovingCubesI (irgendwann Anfang '93). Es hat sich
*       zwar einigermaßen bewährt, die Aufgaben der Engines
*       können aber auch durch normale Klassen übernommen
*       werden, daß wird auch irgendwann mal passieren, also
*       nicht zu sehr drauf versteifen :-)
*
*   BUGS
*
*   SEE ALSO
*       _CloseEngine(), modules.h
*
*   HISTORY
*       31-Dec-94   floh    created
*       02-Feb-95   floh    der Parameter <name> ist weggefallen,
*                           die Routine ermittelt jetzt die zu ladende
*                           Engine automatisch aus dem Standard-Config-File.
*                           (mittels nc_GetConfigItems()).
*       24-Feb-97   floh    Alle Engine-Handles jetzt in NucleusBase
*                           eingebettet
*       13-Apr-97   floh    #?Eng_Name Handling, damit kann die
*                           Init-File-Konfiguration "overriden"
*                           werden.
*
*****************************************************************************
*
*/
#ifdef AMIGA
struct GET_Engine *__asm nc_OpenEngine(__d0 ULONG id)
#else
struct GET_Engine *nc_OpenEngine(ULONG id)
#endif
{
    struct SegmentHandle *seg_handle;
    struct GET_Engine *get;         /* GET der Engine */
    struct GET_Engine **get_ptr;    /* in diesen Pointer GET aufheben */
    UBYTE seg_name[256];
    ULONG result;

    /* ConfigItem-Array für 1 Eintrag (wird weiter unten init.) */
    UBYTE eng_name[CONFIG_MAX_STRING_LEN] = "dummy.engine";
    struct ConfigItem cfg_engine;

    /* ConfigItem-Array teilweise initialisieren */
    cfg_engine.keyword = "dummy.engine";
    cfg_engine.type    = CONFIG_STRING;
    cfg_engine.data    = (LONG) eng_name;

    /* welches glob. Segment-Handle und welchen glob. GET-Pointer? */
    switch (id) {
        case MID_ENGINE_OUTPUT_VISUAL:
            seg_handle = &(NBase.VisOutEng_SH);
            get_ptr    = &(NBase.VisOutEng_GET);
            cfg_engine.keyword = "gfx.engine";
            break;
        case MID_ENGINE_OUTPUT_AUDIO:
            seg_handle = &(NBase.AudOutEng_SH);
            get_ptr    = &(NBase.AudOutEng_GET);
            cfg_engine.keyword = "audio.engine";
            break;
        case MID_ENGINE_INPUT:
            seg_handle = &(NBase.InEng_SH);
            get_ptr    = &(NBase.InEng_GET);
            cfg_engine.keyword = "input.engine";
            break;
        case MID_ENGINE_TRANSFORM:
            seg_handle = &(NBase.TransEng_SH);
            get_ptr    = &(NBase.TransEng_GET);
            cfg_engine.keyword = "tform.engine";
            break;
    };

    /* Engine-Filenamen aus Config-File ermitteln */
    if (!nc_GetConfigItems(NULL, &cfg_engine, 1)) return(NULL);

    /* "echten" Pfadname zusammenbasteln */
    strcpy(seg_name, ASSIGN_engines);   /* "MC2engines:" */

    /* Override-Name oder Name aus Config-File? ***/
    strcat(seg_name, (UBYTE *) cfg_engine.data);

    /* Code-Segment laden */
    get = (struct GET_Engine *) nc_OpenSegment(seg_name,seg_handle);
    if (!get) return(NULL);

    /* GET-Pointer in globalem Pointer aufheben (je nach Engine-Typ) */
    *get_ptr = get;

    /* Open()-Routine der Engine anspringen */
    result = get->Open(MID_NUCLEUS,     &nuc_NUC_GET,
                       MID_NucleusBase, &NBase,
                       TAG_DONE);
    if (!result) {
        nc_CloseSegment(seg_handle);
        return(NULL);
    };

    /* das war's schon */
    return(get);
}

/****** nucleus/_CloseEngine ***************************************
*
*   NAME   
*       _CloseEngine - schließt und entfernt eine geöffnete Engine
*
*   SYNOPSIS
*       _CloseEngine(id)
*
*       void _CloseEngine(ULONG)
*
*   FUNCTION
*       Jede mit _OpenEngine() geöffnete Engine muß mit
*       _CloseEngine() geschlossen werden. _CloseEngine()
*       darf *NICHT* aufgerufen werden, wenn der zugehörige
*       _OpenEngine() Aufruf erfolglos war.
*
*   INPUTS
*       id  - Engine-ID, wie in _OpenEngine() beschrieben.
*
*   RESULT
*       keine
*
*   EXAMPLE
*       siehe _OpenEngine()
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       _OpenEngine(), modules.h
*
*   HISTORY
*      31-Dec-94   floh    created
*      24-Feb-97   floh    alle globalen Handles in NBase eingebettet
*
*****************************************************************************
*
*/
#ifdef AMIGA
void __asm nc_CloseEngine(__d0 ULONG id)
#else
void nc_CloseEngine(ULONG id)
#endif
{
    struct SegmentHandle *seg_handle;
    struct GET_Engine *get;

    switch(id) {
        case MID_ENGINE_OUTPUT_VISUAL:
            seg_handle = &(NBase.VisOutEng_SH);
            get        = NBase.VisOutEng_GET;
            break;
        case MID_ENGINE_OUTPUT_AUDIO:
            seg_handle = &(NBase.AudOutEng_SH);
            get        = NBase.AudOutEng_GET;
            break;
        case MID_ENGINE_INPUT:
            seg_handle = &(NBase.InEng_SH);
            get        = NBase.InEng_GET;
            break;
        case MID_ENGINE_TRANSFORM:
            seg_handle = &(NBase.TransEng_SH);
            get        = NBase.TransEng_GET;
            break;
    };

    /* war Engine gar nicht geöffnet? */
    if (get == NULL) return;

    /* Close() der Engine aufrufen */
    get->Close();

    /* Code-Segment unloaden */
    nc_CloseSegment(seg_handle);

    /* fertig */
}

/****** C_Language/normal ******************************************
*
*   NAME   
*       _SetSysPath -- setzt Nucleus-System-DOS-Pfad
*
*   SYNOPSIS
*       _SetSysPath(id, path)
*
*       void _SetSysPath(ULONG, STRPTR)
*
*
*   FUNCTION
*       Modifiziert einen System-Pfad, die Modifikation
*       bleibt bis zum nächsten _SetSysPath() gültig.
*
*   INPUTS
*       id      - ID des System-Pfads, der modifiziert werden soll:
*                   SYSPATH_RESOURCES
*                   SYSPATH_CLASSES
*                   SYSPATH_ENGINES
*
*       path    - Ptr auf C-String mit neuen Pfad,
*                 ist <path> NULL, wird der Default-Pfad benutzt
*
*   RESULT
*       ---
*
*   HISTORY
*       24-Jan-96   floh    created
*       24-Feb-97   floh    SysPaths jetzt in NBase eingebettet
*
*****************************************************************************
*
*/
#ifdef AMIGA
void __asm nc_SetSysPath(__d0 ULONG id, __a0 STRPTR path)
#else
void nc_SetSysPath(ULONG id, STRPTR path)
#endif
{
    /*** Security Check ***/
    if (id >= NC_NUM_SYSPATHS) return;

    /*** NULL-Pfad? ***/
    if (!path) {
        switch(id) {
            case SYSPATH_RESOURCES:
                path = ASSIGN_resources;
                break;

            case SYSPATH_CLASSES:
                path = ASSIGN_classes;
                break;

            case SYSPATH_ENGINES:
                path = ASSIGN_engines;
                break;

            default: return;
        };
    };

    /*** neuen Pfad setzen ***/
    strcpy(&(NBase.SysPath[id][0]), path);

    /*** Ende ***/
}

/****** C_Language/normal ******************************************
*
*   NAME   
*       _GetSysPath -- ermittle einen Nucleus-System-DOS-Pfad
*
*   SYNOPSIS
*       path = _GetSysPath(id)
*
*       STRPTR _GetSysPath(ULONG)
*
*   FUNCTION
*       Returniert den der übergebenen ID entsprechenden
*       Standard-System-Pfad.
*
*   INPUTS
*       id      - eins von
*                   SYSPATH_RESOURCES
*                   SYSPATH_CLASSES
*                   SYSPATH_ENGINES
*
*   RESULT
*       path    - Pointer auf C-String mit Pfads
*
*   HISTORY
*       24-Jan-96   floh    created
*       24-Feb-97   floh    SysPaths jetzt in NBase eingebettet
*
*****************************************************************************
*
*/
#ifdef AMIGA
STRPTR __asm nc_GetSysPath(__d0 ULONG id)
#else
STRPTR nc_GetSysPath(ULONG id)
#endif
{
    /*** Security Check ***/
    if (id >= NC_NUM_SYSPATHS) return(NULL);
    else return(&(NBase.SysPath[id][0]));
}

