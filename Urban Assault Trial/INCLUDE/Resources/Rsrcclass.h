#ifndef RESOURCES_RSRCCLASS_H
#define RESOURCES_RSRCCLASS_H
/*
**  $Source: PRG:VFM/Include/resources/rsrcclass.h,v $
**  $Revision: 38.4 $
**  $Date: 1996/02/28 23:10:33 $
**  $Locker:  $
**  $Author: floh $
**
**  Die rsrc.class ersetzt die bisherige resource.class.
**
**   (C) Copyright 1994 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif

#ifndef NUCLEUS_NUCLEUS2_H
#include "nucleus/nucleus2.h"
#endif

#ifndef NUCLEUS_NUKEDOS_H
#include "nucleus/nukedos.h"
#endif

#ifndef NUCLEUS_NUCLEUSCLASS_H
#include "nucleus/nucleusclass.h"
#endif

/*-------------------------------------------------------------------
**  Die ClassID der rsrc.class
*/
#define RSRC_CLASSID "rsrc.class"

/*-------------------------------------------------------------------
**  Das <rsrcpool>-Directory (in MC2resources:) ist zur "privaten"
**  Benutzung durch die "resource.class" und ihre Subklassen reserviert.
**  Objekte, die Shared Resources verwalten, können sich dafür
**  entscheiden, bei einem OM_SAVETOIFF diese getrennt vom
**  eigentlichen Diskobject in einem privaten Fileformat ins
**  <rsrcpool>-Directory zu schreiben. Als Filename wird hierfür
**  direkt der Name des Resource-Handles RA_Name verwendet.
**  Durch dieses Vorgehen wird das Resource Sharing zu Laufzeit
**  auch mit ins Filesystem "exportiert". Neben einer Platzeinsparung
**  für Diskobjects verkürzen sich dadurch auch die Ladezeiten
**  ganz erheblich.
*/
#define DISK_RSRCPOOL "rsrcpool/"

/*-------------------------------------------------------------------
**  NAME
**      rsrc.class - verwaltet Speicherbereiche als
**                   (optional shared) Resourcen.
**
**  FUNCTION
**      Neue Shared Resourcen Klasse.
**
**  METHODS
**      OM_NEW
**      OM_DISPOSE
**      OM_GET
**      OM_SET
**      OM_SAVETOIFF
**      OM_NEWFROMIFF
**
**      RSM_CREATE
**          Msg:    struct TagItem[]
**          Ret:    struct RsrcNode *
**
**          Weist ein Objekt an, eine Resource aus einem
**          Datenfile zu erzeugen. Dieser File darf ein
**          Standalone File sein oder Teil einer
**          Resource-Collection, die mit RSM_COLLECT
**          erzeugt wurde.
**
**          Als Msg bekommt die Methode die komplette (I)
**          TagList aus OM_NEW, heraus kommt eine komplette
**          Resource-Node.
**
**      RSM_FREE
**          Msg:    struct rsrc_free_msg
**          Ret:    ---
**
**          Die in der Msg übergebene Resource wird gekillt,
**          der OpenCount wird dabei ignoriert.
**
**      RSM_SAVE
**          Msg:    struct rsrc_save_msg
**          Ret:    eins von RSRC_SAVE_STANDALONE
**                           RSRC_SAVE_COLLECTION
**                           RSRC_SAVE_ERROR
**
**          Das Objekt soll seine Resource in einen Datenfile
**          sichern. Das darf ein Standalone File sein oder
**          Teil eines bereits existierenden Collect-Files.
**          Das Objekt darf sich "weigern", die Methode
**          auszuführen, das tritt z.B. dann auf, wenn das
**          Resource-File-Format nicht IFF entspricht und
**          sich die Resource in einen Collect-File sichern
**          soll (Collect-Files sind immer IFF).
**
**          Aus dem Return-Value läßt sich ermitteln, was
**          das Objekt nu gemacht hat...
**
**          Der Filename wird relativ zu mc2resources:
**          interpretiert!
**
**  ATTRIBUTES:
**
**      RSA_Name    (IG) - UBYTE *
**          Eindeutiger Name der Resource. Im Normal-
**          fall wird dies der Filename eines
**          Resource-Files sein, aus dem das Objekt
**          "seine" Resource erzeugen soll. Der Name
**          ist gleichzeitig eine eindeutige ID der
**          Resource und darf für verschiedene Resourcen
**          nicht identisch sein.
**
**      RSA_Access  (IG) - [ACCESS_SHARED|ACCESS_EXCLUSIVE]
**          Definiert, ob sich mehrere Client-Objects
**          diese Resource teilen dürfen oder ob nur
**          ein Client-Object (nämlich der Erzeuger) diese
**          Resource nutzen darf.
**
**      RSA_Handle  (G) - void *
**          Definiert den Pointer auf das eigentliche
**          Resource-Handle. Der genaue Typ des Pointers
**          ist Subklassen-spezifisch.
**
**      RSA_IFFHandle (I) - struct IFFHandle *
**          Ein gültiges IFFHandle zeigt an, daß die
**          Resource aus einem bereits offenen Collect-File
**          geladen werden soll. Das IFFHandle definiert
**          diesen Collect-File.
**
**      RSA_DontCopy  (IG) - BOOL
**          Falls TRUE, wird das Objekt angewiesen, diverse
**          extern bereitgestellte Daten nicht zu kopieren,
**          sondern nur einen Verweis darauf zu halten.
**          Die rsrc.class selbst speichert nur den Status
**          von RSA_DontCopy, verwendet es aber selbst nicht.
**          Subklassen dürfen dies aber nach Belieben. 
**          NUR FÜR DEN INTERNEN GEBRAUCH!!!
**
**      RSA_FromDiskRsrc (I) - BOOL
**          Falls TRUE, wird die zum Object gehörende Resource
**          nicht aus den Init-Attributen, sondern durch Einladen
**          von Disk (aus dem MC2resources:rsrcpool-Directory)
**          erzeugt. RA_Name entpricht dabei dem Filenamen
**          relativ zu MC2resources:rsrcpool.
**          Die resource.class bietet dieses Attribut nur als
**          virtuelles Attribut an, Subklassen können damit
**          etwas anfangen, müssen aber nicht. DIE GESAMTE
**          IMPLEMENTATION IST ZUR ZEIT EXPERIMENTELL, ALSO
**          BITTE NICHT "VON AUßEN" BENUTZEN!
**
**      RSA_SharedRsrcList  (G) - struct MinList *
**          Returniert einen Pointer auf die globale
**          Liste der Shared-Resources. Diese Liste
**          besteht aus <struct RsrcNode>-Elementen und
**          ist READ ONLY!
**
**      RSA_ExclusiveRsrcList (G) - struct MinList *
**          Returniert Pointer auf Liste der
**          Exclusive-Resources. READ ONLY!
**
**      RSA_Weight  (private) ULONG [internal use only!]
**          Dieses Attribut wird nur innerhalb RSM_CREATE
**          ausgewertet, und gibt an, welche "Wichtung"
**          die Resource besitzt. Das natürlich nur im
**          übertragenden Sinn. Eine Resource ist
**          umso schwerer, je mehr "rekursive" Subresourcen
**          sie besitzt, ein Beispiel:
**
**          Ein normales ilbm.class oder sklt.class Objekt
**          hat die Wichtung 0, weil sie keine Subresourcen
**          benutzen.
**
**          Ein bmpanim.class Objekt hat dagegen die 
**          Wichtung 1, weil es bitmap.class Objects als
**          Subresourcen benötigt (ein bmpanim.class Object
**          mit anderen Anims als Subresourcen hätte theoretisch
**          Wichtung 2).
**
**          Die Wichtung ist wichtig zum Einsortieren in die
**          Resource-Liste, je größer die Wichtung, desto
**          weiter hinten wird die Resource einplaziert, damit
**          beim linearen Durcharbeiten der Liste von vorn
**          garantiert ist, daß alle Subresourcen einer
**          "schwereren" Resource bereits abgearbeitet wurden.
**          Konkret auf das Problem gestoßen bin ich mit
**          der embed.class, die die Resource-Liste als
**          linearen Chunk sichert.
**
**          WICHTIG: Zur Zeit werden nur getestet, ob die
**          Wichtung 0 beträgt (in diesem Fall wird die
**          Resource von vorn an die Liste gehängt), oder
**          ob sie größer 0 ist (dann wird _AddTail()) verwendet.
**          Eine Einsortierung findet also nicht statt!
*/

#define RSRC_NAME_CHARS (64)

#define ACCESS_SHARED       (1)
#define ACCESS_EXCLUSIVE    (2)

#define RSRC_SAVE_ERROR         (0)
#define RSRC_SAVE_STANDALONE    (1)
#define RSRC_SAVE_COLLECTION    (2)

/*-------------------------------------------------------------------
**  Die Methoden- und Attribute-Definitionen
*/
#define RSM_BASE         (OM_BASE+METHOD_DISTANCE)
#define RSM_CREATE       (RSM_BASE)
#define RSM_FREE         (RSM_BASE+1)
#define RSM_SAVE         (RSM_BASE+2)
/*-------------------------------------------------------------------
**  Die Attribute wurden so gewählt, dass sie vorsichtshalber
**  mit denen der alten resource.class Binaer-kompatibel sind.
*/
#define RSA_BASE         (OMA_BASE+ATTRIB_DISTANCE)
#define RSA_Name                (RSA_BASE)      // (IG)
#define RSA_Access              (RSA_BASE+1)    // (IG)
#define RSA_Handle              (RSA_BASE+2)    // (G)
#define RSA_DontCopy            (RSA_BASE+3)    // (IG)
#define RSA_FromDiskRsrc        (RSA_BASE+4)    // (I)
#define RSA_IFFHandle           (RSA_BASE+5)    // (I)
#define RSA_SharedRsrcList      (RSA_BASE+6)    // (G)
#define RSA_ExclusiveRsrcList   (RSA_BASE+7)    // (G)
#define RSA_Weight              (RSA_BASE+8)    // (private)

//
// Kompatibilität mit alter resource.class
//
#define RA_Name         RSA_Name
#define RA_Access       RSA_Access
#define RA_Handle       RSA_Handle
#define RA_DontCopy     RSA_DontCopy
#define RA_FromDiscRsrc RSA_FromDiscRsrc
#define RA_IFFHandle    RSA_IFFHandle

/*-------------------------------------------------------------------
**  Die rsrc.class verwaltet eine interne Liste aller Shared
**  Resources. Die Elemente dieser Liste sind <struct RsrcNode>'s.
*/
struct RsrcNode {
    struct Node Node;   // ln_Name besetzt, ln_Type>RType_DontCopy
    UBYTE Name[RSRC_NAME_CHARS];    // ln_Name zeigt hier drauf
    UWORD OpenCount;
    UWORD Access;           // ACCESS_SHARED||ACCESS_EXCLUSIVE
    void *Handle;           // abstraktes Handle (z.B. struct VFMBitmap *)
    UBYTE *HandlerClass;    // (z.B. "ilbm.class")
};

/*-------------------------------------------------------------------
**  Die Locale Instance Data der rsrc.class
*/
struct rsrc_data {
    struct RsrcNode *rnode;     // NULL, falls exclusive
    void *handle;               // direkter Pointer auf Handle
    ULONG flags;
};

/*-------------------------------------------------------------------
**  Defs für rsrc_data.flags
*/
#define RSF_DontCopy    (1<<0)

/*-------------------------------------------------------------------
**  Definitionen für resource_data.flags
*/
#define RF_DontCopy (1<<0)  /* nicht verwechseln mit RType_DontCopy!!! */

struct rsrc_save_msg {
    UBYTE *name;            // nur, wenn (type == RSRC_SAVE_STANDALONE)
    struct IFFHandle *iff;  // nur, wenn (type == RSRC_SAVE_COLLECTION)
    ULONG type;             // RSRC_SAVE_STANDALONE|RSRC_SAVE_COLLECTION
};

struct rsrc_free_msg {
    struct RsrcNode *rnode; // Ptr auf freizugebende Resource
};

/*-----------------------------------------------------------------*/
#endif

