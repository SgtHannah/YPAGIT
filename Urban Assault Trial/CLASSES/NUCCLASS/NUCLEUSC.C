/*
**  $Source: PRG:VFM/Classes/_NucleusClass/nucleusclass.c,v $
**  $Revision: 38.10 $
**  $Date: 1995/02/14 20:52:50 $
**  $Locker:  $
**  $Author: floh $
**
**  Die Rootclass des Nucleus-Systems. It all begins here :-)
**
**  (C) 1993 Copyright by A.Weissflog
*/
#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <libraries/iffparse.h>

#include <string.h>

#include "nucleus/nucleus2.h"
#include "nucleus/nukedos.h"
#include "nucleus/class.h"
#include "nucleus/nucleusclass.h"
#include "modules.h"

/*-------------------------------------------------------------------
**  Methoden-Dispatcher-Prototypen
*/
_dispatcher(Object *, nucleus_OM_NEW, struct TagItem *attrs);
_dispatcher(BOOL, nucleus_OM_DISPOSE, void *ignore);
_dispatcher(void, nucleus_OM_SET, struct TagItem *attrs);
_dispatcher(void, nucleus_OM_GET, struct TagItem *attrs);
_dispatcher(Object *, nucleus_OM_NEWFROMIFF, struct iff_msg *iffmsg);
_dispatcher(BOOL, nucleus_OM_SAVETOIFF, struct iff_msg *iffmsg);

/*-------------------------------------------------------------------
**  Globals
*/
#ifdef AMIGA
__far ULONG nuc_Methods[NUCLEUS_NUMMETHODS];
#else
ULONG nuc_Methods[NUCLEUS_NUMMETHODS];
#endif

struct ClassInfo ClassInfo;

#ifdef DYNAMIC_LINKING
struct GET_Nucleus *NUC_GET;
#endif

/*-------------------------------------------------------------------
**  Global Entry Table des Klassen-Moduls + Prototypes
*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeNucleusClass(ULONG id,...);
__geta4 BOOL FreeNucleusClass(void);
#else
struct ClassInfo *MakeNucleusClass(ULONG id,...);
BOOL FreeNucleusClass(void);
#endif

struct GET_Class nucleus_GET = {
    &MakeNucleusClass,
    &FreeNucleusClass,
};

/*===================================================================
**  *** CODESEGMENT HEADER ***
*/
#ifdef DYNAMIC_LINKING
#ifdef AMIGA
/* spezieller hocheffizienter DICE-Startup-Code */
__geta4 struct GET_Class *start(void)
{
    return(&nucleus_GET);
}
#endif
#endif

#ifdef STATIC_LINKING
/* die Einsprung-Routine */
struct GET_Class *nucleus_Entry(void)
{
    return(&nucleus_GET);
};

/* und die zugehörige SegmentInfo-Struktur */
struct SegInfo nucleus_class_seg = {
    { NULL, NULL,                   /* ln_Succ, ln_Pred */
      0, 0,                         /* ln_Type, ln_Pri  */
      "MC2classes:nucleus.class"    /* der Segment-Name */
    },
    nucleus_Entry,  /* Entry()-Adresse */
};

#endif  /* STATIC_LINKING */

/*-------------------------------------------------------------------
**  Die DummyMethod() ist für die "leeren" Methoden. Jede SubClass
**  der Nucleus-Class downloaded diese Dummy-Method in ihre eigenen
**  nicht unterstützten Methoden.
*/
ULONG DummyMethod(void)
{
    return(0);
}

#ifdef DYNAMIC_LINKING
/*-------------------------------------------------------------------
**  Logischerweise kann der NUC_GET-Pointer nicht mit einem
**  _GetTagData() aus dem Nucleus-Kernel ermittelt werden,
**  weil NUC_GET für den Einsprung geholt wird! Deshalb hier eine
**  handgestrickte Routine.
*/
struct GET_Nucleus *local_GetNucleus(struct TagItem *tagList)
{
    register ULONG act_tag;

    while ((act_tag = tagList->ti_Tag) != MID_NUCLEUS) {
        switch (act_tag) {
            case TAG_DONE:  return(NULL); break;
            case TAG_MORE:  tagList = (struct TagItem *) tagList->ti_Data; break;
            case TAG_SKIP:  tagList += tagList->ti_Data; break;
        };
        tagList++;
    };
    return((struct GET_Nucleus *)tagList->ti_Data);
}
#endif /* DYNAMIC_LINKING */

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeNucleusClass(ULONG id,...)
#else
struct ClassInfo *MakeNucleusClass(ULONG id,...)
#endif
/*
**  FUNCTION
**      Gibt ausgefüllte ClassInfo-Struktur für MakeClass()
**      zurück.
**
**  INPUTS
**      Aus der InitTag-List wird der Pointer auf das NUC_GET geholt.
**
**  RESULTS
**      ausgefüllte ClassInfo-Struktur für Nucleus-Class, oder
**      NULL, wenn ein Fehler auftrat.
**
**  CHANGED
**      18-Nov-93   floh    created
**      14-Dec-94   floh    (1) __geta4
**                          (2) neu: OM_SET und OM_GET Dispatcher
**                              (wegen neuem OA_Name-Attribut).
**      01-Jan-94   floh    jetzt nach Nucleus2-Standard
*/
{
    ULONG i;

    #ifdef DYNAMIC_LINKING
    /* NUC_GET-Pointer holen */
    if (!(NUC_GET = local_GetNucleus((struct TagItem *)&id))) return(NULL);
    #endif

    /* Methoden-Array mit Dummy-Method füllen */
    for (i=0; i<NUCLEUS_NUMMETHODS; i++) {
        nuc_Methods[i] = (ULONG) &DummyMethod;
    };

    /* OM-Methoden eintragen */
    nuc_Methods[OM_NEW]         = (ULONG) nucleus_OM_NEW;
    nuc_Methods[OM_DISPOSE]     = (ULONG) nucleus_OM_DISPOSE;
    nuc_Methods[OM_SET]         = (ULONG) nucleus_OM_SET;
    nuc_Methods[OM_GET]         = (ULONG) nucleus_OM_GET;
    nuc_Methods[OM_SAVETOIFF]   = (ULONG) nucleus_OM_SAVETOIFF;
    nuc_Methods[OM_NEWFROMIFF]  = (ULONG) nucleus_OM_NEWFROMIFF;

    /* ClassInfo-Struktur ausfüllen und zurückgeben */
    ClassInfo.superclassid = NULL;
    ClassInfo.methods      = nuc_Methods;
    ClassInfo.instsize     = sizeof(struct nucleusdata);
    ClassInfo.flags        = 0;

    return(&ClassInfo);
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 BOOL FreeNucleusClass(void)
#else
BOOL FreeNucleusClass(void)
#endif
/*
**  FUNCTION
**      Gegenstück zu MakeNucleusClass()
**
**  INPUTS
**      ---
**
**  RESULTS
**      immer TRUE
**
**  CHANGED
**      18-Dec-93   floh    created
**      14-Dec-94   floh    __geta4
**      01-Jan-94   floh    jetzt nach Nucleus2-Standard
*/
{
    return(TRUE);
}

/*===================================================================**
**  SUPPORT ROUTINEN                                                 **
**===================================================================*/
/*-----------------------------------------------------------------*/
void initAttributes(Object *o, struct nucleusdata *nd, struct TagItem *attrs)
/*
**  FUNCTION
**      Initialisiert LID mit (I)-Attributen.
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      14-Dec-94   floh    created
**      01-Jan-95   floh    jetzt nach Nucleus2-Standard
*/
{
    register ULONG tag;

    /* Attribut-Liste scannen */
    while ((tag = attrs->ti_Tag) != TAG_DONE) {

        register ULONG data = attrs++->ti_Data;

        switch(tag) {

            /* die System-Tags... */
            case TAG_IGNORE:    continue;
            case TAG_MORE:      attrs = (struct TagItem *) data; break;
            case TAG_SKIP:      attrs += data; break;
            default:

                /* dann die "echten" Attribute */
                switch (tag) {

                    case OA_Name:

                        /* falls OA_Name NICHT NULL-Ptr */
                        if (data) {

                            /* allokiere 32 Byte Buffer */
                            nd->o_Name = _AllocVec(SIZEOF_NAME+1,MEMF_PUBLIC);
                            if (nd->o_Name) {
                                strncpy(nd->o_Name,(char *)data,SIZEOF_NAME);
                            };
                        };
                        break;
                };
        };
    };

    /* das war's */
}

/*-----------------------------------------------------------------*/
void setAttributes(Object *o, struct nucleusdata *nd, struct TagItem *attrs)
/*
**  FUNCTION
**      Setzt (S)-Attribute
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      14-Dec-94   floh    created
**      01-Jan-95   floh    jetzt nach Nucleus2-Standard
*/
{
    register ULONG tag;

    /* Attribut-Liste scannen */
    while ((tag = attrs->ti_Tag) != TAG_DONE) {

        register ULONG data = attrs++->ti_Data;

        switch(tag) {

            /* die System-Tags... */
            case TAG_IGNORE:    continue;
            case TAG_MORE:      attrs = (struct TagItem *) data; break;
            case TAG_SKIP:      attrs += data; break;
            default:

                /* dann die "echten" Attribute */
                switch (tag) {

                    case OA_Name:

                        /* Name-Flush? */
                        if (data == NULL) {
                            /* Namens-Buffer killen */
                            if (nd->o_Name) {
                                _FreeVec(nd->o_Name);
                                nd->o_Name = NULL;
                            };
                        } else {
                            /* falls kein Namens-Buffer ex, einen allok. */
                            if (nd->o_Name == NULL) {
                                nd->o_Name = _AllocVec(SIZEOF_NAME+1,MEMF_PUBLIC);
                                if (nd->o_Name) {
                                    strncpy(nd->o_Name,(char *)data,SIZEOF_NAME);
                                };
                            } else {
                                strncpy(nd->o_Name,(char *)data,SIZEOF_NAME);
                            };
                        };
                        break;
                };
        };
    };

    /* das war's */
}

/*-----------------------------------------------------------------*/
void getAttributes(Object *o, struct nucleusdata *nd, struct TagItem *attrs)
/*
**  FUNCTION
**      Abfrage der (G)-Attribute.
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      14-Dec-94   floh    created
**      20-Dec-94   floh    neu: OA_Class und OA_ClassName
**      01-Jan-95   floh    jetzt nach Nucleus2-Standard
*/
{
    register ULONG tag;

    /* Attribut-Liste scannen */
    while ((tag = attrs->ti_Tag) != TAG_DONE) {

        register ULONG *value = (ULONG *) attrs++->ti_Data;

        switch (tag) {

            /* die System-Tags... */
            case TAG_IGNORE:    continue;
            case TAG_MORE:      attrs = (struct TagItem *) value; break;
            case TAG_SKIP:      attrs += (ULONG) value; break;
            default:

                /* dann die "echten" Attribute */
                switch (tag) {

                    case OA_Name:
                        *value = (ULONG) nd->o_Name;
                        break;

                    case OA_Class:
                        *value = (ULONG) nd->o_Class;
                        break;

                    case OA_ClassName:
                        *value = (ULONG) nd->o_Class->node.ln_Name;
                        break;
                };
        };
    };

    /* das war's */
}

/*===================================================================**
**  METHODEN-DISPATCHER                                              **
**===================================================================*/

/*-------------------------------------------------------------------*/
_dispatcher(Object *, nucleus_OM_NEW, struct TagItem *attrs)
/*
**  FUNCTION
**      OM_NEW-Dispatcher der Nucleus-Class.
**
**  INPUTS
**      o       - hier steht kein Object-Pointer drin
**                (welcher denn auch), sondern der von
**                NewObject() hier reingeschriebene und von
**                allen sub-OM_NEW-Dispatchern weitergegebene
**                Pointer auf die "TrueClass".
**      cl      - Pointer auf nucleus-Class
**      attrs   - (I)-Attribute
**
**  RESULTS
**      Object *    - Pointer auf allokierte Object-Struktur, oder NULL,
**                    wenn nicht genug Speicher.
**
**  CHANGED
**      ??-Aug-93   floh    created
**      06-Sep-93   floh    Bug: Object-Size wurde falsch berechnet - fixed
**      18-Dec-93   floh    aus Nucleus.c übernommen nach nucleusclass.c,
**                          die nucleusclass ist jetzt eine normale DiskClass
**      14-Dec-94   floh    (1) __geta4
**                          (2) ruft jetzt initAttributes() auf
**      01-Jan-95   floh    jetzt nach Nucleus2-Standard.
*/
{
    ULONG osize;
    Object *newo;

    /* zuerst in der TrueClass gucken, wie groß ich werde */
    if (((Class *)o)->superclass) {
        /* wir werden Instanz einer Sub-Klasse der nucleusclass */
        osize = ((Class *)o)->instsize + ((Class *)o)->instoffset;
    } else {
        /* sonst Instanz der nucleusclass */
        osize = ((Class *)o)->instsize;
    };

    /* Object allokieren */
    newo = (Object *) _AllocVec(osize, MEMF_PUBLIC|MEMF_CLEAR);
    if (!newo)  return(NULL);

    /* das hat geklappt, also ObjectCount der TrueClass inkrementieren */
    ((Class *)o)->objectcount += 1;

    /* und den Pointer auf die TrueClass ins Object... */
    ((struct nucleusdata *)newo)->o_Class = (Class *) o;

    /* jetzt noch die Attribute initialisieren */
    initAttributes(newo,(struct nucleusdata *)newo,attrs);

    /* das war alles */
    return(newo);
}

/*-------------------------------------------------------------------*/
_dispatcher(BOOL, nucleus_OM_DISPOSE, void *ignore)
/*
**  FUNCTION
**      Dispose-Dispatcher der Nucleus-Class, entfernt das
**      Object aus dem Nucleus-System (und dem Speicher).
**
**  INPUTS
**      o       - Pointer auf zu disposendes Object
**      cl      - Pointer auf Object-Klasse
**      ignore  - wird ignoriert
**
**  RESULTS
**      BOOL = TRUE, wenn alles glatt ging (eigentlich immer)
**
**  CHANGED
**      18-Dec-93   floh    erzeugt aus OM_DISPOSE in nucleus.c
**      14-Dec-94   floh    (1) __geta4
**                          (2) falls vorhanden, wird Namens-Buffer
**                              o_Name freigegeben
**      01-Jan-95   floh    jetzt nach Nucleus2-Standard
*/
{
    /* Da die nucleusclass die RootClass ist, brauche ich kein       */
    /* INST_DATA() anzuwenden, um an die Locale Instance Data zu     */
    /* kommen. o zeigt sowieso schon drauf.                          */

    /* Klasse holen und ObjectCount decremtieren */
    ((struct nucleusdata *)o)->o_Class->objectcount -= 1;

    /* dann FreeVec'n */
    if (((struct nucleusdata *)o)->o_Name) {
        _FreeVec(((struct nucleusdata *)o)->o_Name);
    };
    _FreeVec(o);

    return(TRUE);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, nucleus_OM_SET, struct TagItem *attrs)
/*
**  FUNCTION
**      OM_SET-Dispatcher der nucleus.class
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      14-Dec-94   floh    created
**      01-Jan-95   floh    jetzt nach Nucleus2-Standard
*/
{
    /* Attribute setzen */
    setAttributes(o,(struct nucleusdata *)o,attrs);

    /* fertig */
}

/*-----------------------------------------------------------------*/
_dispatcher(void, nucleus_OM_GET, struct TagItem *attrs)
/*
**  FUNCTION
**      OM_GET-Dispatcher der nucleus.class
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      14-Dec-94   floh    created
**      01-Jan-94   floh    jetzt nach Nucleus2-Standard
*/
{
    /* Attribute getten */
    getAttributes(o,(struct nucleusdata *)o,attrs);

    /* fertig */
}

/*-----------------------------------------------------------------*/
_dispatcher(Object *, nucleus_OM_NEWFROMIFF, struct iff_msg *iffmsg)
/*
**  FUNCTION
**      Das ist der "Stamm"-Dispatcher zum Erzeugen eines
**      Objects aus einem Datenfile. Der Dispatcher liest
**      ausschließlich IFF-Chunks, die mit nucleus_OM_SAVETOIFF()
**      erzeugt wurden.
**
**  INPUTS
**      o       -> Hier steht *KEIN* Object-Pointer drin,
**                 ganz einfach deshalb, weil zum Zeitpunkt
**                 der Anwendung noch gar kein Object existiert.
**                 Vielmehr wird hier von nc_LoadObject()
**                 der Pointer auf die TrueClass hochgereicht.
**                 nc_LoadObject() ist übrigens die einzig legitime
**                 Methode, ein Object aus einem Datenfile zu
**                 erzeugen.
**      cl      -> Pointer auf Nucleus-Class (wird von
**                 DoSuperMethod()) erledigt.
**      iff     -> Pointer auf IFF-Message.
**
**  RESULTS
**      Object * -> Pointer auf komplettes Object oder NULL bei Mißerfolg
**
**  CHANGED
**      24-Sep-93   floh    created
**      18-Dec-93   floh    übernommen aus nucleus.c
**      14-Dec-94   floh    (1) __geta4
**                          (2) akzeptiert jetzt optionalen 
**                              NUCIFF_Name-Chunk
**                          (3) debugging
**      01-Jan-95   floh    jetzt nach Nucleus2-Standard
*/
{
    /* IFFHandle aus iff_msg holen */
    ULONG ifferror;
    struct ContextNode *cn;
    struct IFFHandle *iff = iffmsg->iffhandle;

    /* Größe des Objects ermitteln */
    ULONG osize = ((Class *)o)->instsize + ((Class *)o)->instoffset;

    /* Object allokieren */
    Object *newo = (Object *) _AllocVec(osize, MEMF_PUBLIC|MEMF_CLEAR);
    if (!newo) return(NULL);

    /* Object-Count der TrueClass inc. */
    ((Class *)o)->objectcount += 1;

    /* Klassen-Pointer ins Object eintragen */
    ((struct nucleusdata *)newo)->o_Class = (Class *) o;


    /* eigenen IFF-Chunk scannen */
    while ((ifferror = _ParseIFF(iff, IFFPARSE_RAWSTEP)) != IFFERR_EOC) {
        /* andere Errors als EndOfChunk abfangen */
        if (ifferror)  {
            _doa(newo,OM_DISPOSE,NULL);
            return(NULL); 
        };

        cn = _CurrentChunk(iff);

        /* Namens-Chunk ? */
        if (cn->cn_ID == NUCIFF_Name) {
            /* Chunk-Inhalt lesen */
            UBYTE name[SIZEOF_NAME+1] = { 0 };
            if (!_ReadChunkBytes(iff, name, SIZEOF_NAME)) {
                _doa(newo,OM_DISPOSE,NULL);
                return(NULL);
            };
            /* und ein OM_SET auf mich selbst... */
            _set(newo,OA_Name,name);

            /* EndOfChunk holen */
            _ParseIFF(iff,IFFPARSE_RAWSTEP);
            continue;
        };

        /* unbekannte Chunks überspringen */
        _SkipChunk(iff);
    };

    /* sonst alles klaro... */
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, nucleus_OM_SAVETOIFF, struct iff_msg *iffmsg)
/*
**  FUNCTION
**      "Stamm"-Dispatcher der OM_SAVETOIFF-Methode.
**      WICHTIG: OM_SAVETOIFF darf *NIE* direkt via
**               DoMethod() angewendet werden. Es *MUSS*
**               SaveObject() verwendet werden, da diese
**               Routine die "IFF-Schale" aufbaut und den
**               ganzen IFF-Initialisierungs-Kram übernimmt.
**
**  INPUTS
**      Entsprechend Nucleus-Dispatcher-Interface-Standard.
**      Die Message ist vom Typ >iff_msg<, wie in nucleus/nukedos.h
**      definiert.
**
**  RESULTS
**      TRUE  -> wenn alles geklappt hat
**      FALSE -> Wenn ein Fehler auftrat. Der halbfertige File
**               darf NICHT weiterverwendet werden. Es kann sein,
**               das SaveObject() in diesem Fall den
**               Krüppel-File löscht.
**
**  CHANGED
**      24-Sep-93   floh    created
**      18-Dec-93   floh    übernommen aus nucleus.c
**      14-Dec-94   floh    (1) __geta4
**                          (2) Schreibt jetzt NUCIFF_Name-Chunk,
**                              falls OA_Name einen gültigen Namen hält.
**      01-Jan-95   floh    jetzt nach Nucleus2-Standard
*/
{
    struct IFFHandle *iff   = iffmsg->iffhandle;
    struct nucleusdata *nd = (struct nucleusdata *) o;

    /* FORM ROOT schreiben */
    if (_PushChunk(iff, OF_NUCLEUS, ID_FORM, IFFSIZE_UNKNOWN) != 0)
        return(FALSE);

    /* optionalen NUCIFF_Name-Chunk schreiben */
    if (nd->o_Name) {

        /* Name OHNE 0 speichern */
        _PushChunk(iff,0L,NUCIFF_Name,IFFSIZE_UNKNOWN);
        _WriteChunkBytes(iff,nd->o_Name,strlen(nd->o_Name));
        _PopChunk(iff);
    };

    /* FORM ROOT schließen */
    _PopChunk(iff);

    /* ...und fertig! */
    return(TRUE);
}

