#ifndef NUCLEUS_NUCLEUSCLASS_H
#define NUCLEUS_NUCLEUSCLASS_H
/*
**  $Source: PRG:VFM/Include/nucleus/nucleusclass.h,v $
**  $Revision: 38.8 $
**  $Date: 1994/12/20 19:10:39 $
**  $Locker:  $
**  $Author: floh $
**
**  16-Sep-93   floh    37.10   OM_NEWFROMIFF
**                              OM_SAVETOIFF
**                              OM_DUPLICATE
**                              IFF-FORM-/CHUNK-IDs
**  02-Oct-93   floh    38.1    Revisionskontrolle an RCS �bergeben
**
**  Hier ist die Instance-Data und die unterst�tzten Methoden
**  der >nucleusclass< definiert, was die RootClass des
**  Nucleus-Systems darstellt.
**
**  (C) Copyright 1993 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef LIBRARIES_IFFPARSE_H
#include <libraries/iffparse.h>
#endif

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#ifndef NUCLEUS_CLASS_H
#include "nucleus/class.h"
#endif

/*-------------------------------------------------------------------
**  NAME
**      nucleus.class -- Rootclass des Nucleus-Systems
**
**  FUNCTION
**      Die nucleus.class ist die Rootclass des Nucleus-OOP-Systems.
**      Sie ist damit die Superklasse ALLER sonstigen Klassen und
**      vererbt an diese die elementaren Objekt-Funktionen, wie
**      Konstruktor und Destruktor, Setzen und Abfragen von Attributen
**      und "externe Kapselung" in einen IFF-Datenstrom.
**
**      Falls das Include zusammen mit Standard-Intuition-Includes
**      verwendet werden soll (BOOPSI), bitte das Symbol
**      NUCLEUS_BOOPSI_FRIENDLY definieren!
**
**  METHODS
**      nucleus.class
**      ~~~~~~~~~~~~~
**      OM_NEW
**          Message:    TagList of (I) attributes
**
**          Result:     Object * (Abstraktes Objekt-Handle)
**                               NULL bei Mi�erfolg.
**
**          OM_NEW ist der Konstruktor, der ein neues "Objekt"
**          erzeugt. Eine abgeleitete Klasse sollte hier alle
**          Resourcen allokierenund initialisieren, die ein 
**          neues Objekt ben�tigt.
**          In der �bergebenen TagList k�nnen Initialisierungs-
**          Attribute definiert werden, die den Anfangs-Zustand
**          des neuen Objekts n�her beschreiben. Dabei werden
**          nur die mit (I) gekennzeichneten Attribute
**          akzeptiert.
**          Zur�ckgegeben wird ein Pointer auf das neue Objekt.
**          Hier mu� IMMER getestet werden, ob die Operation
**          erfolgreich war. Bei einem Fehler wird ein NULL-Pointer
**          zur�ckgegeben. Ein Weiterarbeiten mit einem NULL-Objekt
**          f�hrt garantiert zu unerw�nschten Ergebnissen.
**
**          ***WICHTIG*** Diese Methode wird durch die Nucleus-Routine
**          "NewObject()" eingekapselt. Ein direktes Anwenden
**          der Methode auf ein Objekt ist verboten.
**
**          Prototype von NewObject():
**              Object *NewObject(UBYTE *class, ULONG attrs,...);
**
**          Beispiel:
**              Object *o = NewObject("yppsn.class",
**                                      YPPSA_Yoghurt, 1,
**                                      YPPSA_Name,    "schnulli",
**                                      TAG_DONE);
**
**
**      OM_DISPOSE
**          OM_DISPOSE ist der Destruktor. Ein existierendes
**          Objekt wird also vollst�ndig gekillt.
**
**          ***WICHTIG*** Diese Methode darf nie direkt auf ein
**          Objekt angewendet werden!!! Der App-Programmierer wird
**          davor durch die Nucleus-Funktion "DisposeObject()"
**          abgeschirmt.
**
**          Wichtig f�r Klassendesigner: Der Methoden-Dispatcher
**          mu� auch mit unvollst�ndig initialisierten Objekten
**          fertigwerden!
**
**          Prototype von DisposeObject():
**              void DisposeObject(Object *object)
**
**          Beispiel:
**              if (o) DisposeObject(o);
**
**
**      OM_SET
**          Message:    TagList of (S) attributes
**
**          Mit dieser Methode k�nnen (S)-Attribute beeinflu�t
**          werden. Als Message wird eine TagList mit beliebig
**          vielen (S)-Attributen �bergeben.
**
**          Beispiel:
**              DoMethod(o, OM_SET, YPPSA_Yoghurt, 3,
**                                  YPPSA_Name,    "butt_head",
**                                  TAG_DONE);
**
**
**      OM_GET
**          Message:    TagList of (G) attributes
**
**          Hiermit kann der aktuelle Status von (G)-Attributen
**          abgefragt werden. Als Message wird eine TagList
**          mit den gew�nschten Attributen gekoppelt mit
**          ULONG-Pointern �bergeben. Der jeweilige Status des
**          Attributs wird dann in das entsprechende ULONG
**          geschrieben.
**
**          Beispiel:
**              ULONG yoghurt, name;
**              DoMethod(o, OM_GET, YPPSA_Yoghurt, &yoghurt,
**                                  YPPSA_Name,    &name,
**                                  TAG_DONE);
**              if (yoghurt==3) puts(name);
**
**
**      OM_SAVETOIFF
**          Message:    struct iff_msg
**
**          Result:     BOOL success
**
**          Anweisung an ein Objekt, seinen aktuellen Status in
**          einen IFF-Chunk zu sichern. Jeder OM_NEWFROMIFF -Dispatcher
**          in der Klassen-Hierarchie sollte die "w�rdigen"
**          Objekt-Parameter in seinen Chunk schreiben. Welches
**          Format er genau verwendet, ist dabei seine Sache.
**          Der beim OM_SAVETOIFF-Proze� entstehende File enth�lt
**          in seiner Struktur alle Informationen, die zu einem
**          sp�teren Restaurieren des Objekts ben�tigt werden.
**          Dabei mu� nur der Name des Objekt-Files bekannt sein,
**          weitere Informationen werden nicht ben�tigt.
**
**          ***WICHTIG*** Die Methode OM_SAVETOIFF darf nie
**          direkt auf ein Objekt angewendet werden. Daf�r
**          gibt es die Nucleus-Routine "SaveObjectToDisk()".
**          Es ist aber auch m�glich, da� andere Nucleus-Routinen
**          von der OM_SAVETOIFF-Methode Gebrauch machen, zB.
**          f�r Cut-Copy-Paste-Support etc.
**
**          Prototype f�r SaveObjectToDisk():
**              BOOL SaveObjectToDisk(Object *o, UBYTE *filename);
**
**      OM_NEWFROMIFF
**          Message:    struct iff_msg
**
**          Result:     Object * (NULL bei Mi�erfolg)
**
**          OM_NEWFROMIFF restauriert ein Objekt aus einem mit
**          OM_SAVETOIFF erzeugten Objekt-File. OM_NEWFROMIFF
**          stellt sozusagen eine Sonderform von OM_NEW dar, der
**          Unterschied ist, da� OM_NEWFROMIFF seine Initialisierungs-
**          Informationen aus einem IFF-Chunk bekommt, nicht aus einer
**          TagList. Ein OM_NEWFROMIFF -Dispatcher mu� jeweils nur
**          das IFF-Chunk-Format seines "Gegenst�cks" OM_SAVETOIFF
**          im selben Level der Klassenhierarchie kennen.
**
**          Ein mit OM_NEWFROMIFF erzeugtes Objekt kann ganz normal
**          mit OM_DISPOSE [bzw. DisposeObject()] gekillt werden.
**
**          ***WICHTIG***: Die Methode darf nie direkt auf ein
**          Objekt angewendet werden. Um ein Objekt aus einen
**          IFF-DOS-File zu restaurieren, gibt es die Nucleus-Routine
**          "NewDiskObject()".
**
**          Prototype von NewDiskObject():
**              Object *NewDiskObject(UBYTE *filename);
**
**          Den Object-Pointer immer gegen NULL testen (falls
**          etwas schiefging)!
**
**  ATTRIBUTES
**      nucleus.class
**      ~~~~~~~~~~~~~
**      OA_Name (ISG)   <UBYTE *>
**          Definiert einen optionalen Namen f�r das Object. Zur Zeit
**          (14-Dec-94) k�mmert sich die nucleus.class nicht um diesen
**          Namen, das kann sich allerdings in Zukunft �ndern.
**          Intern werden nur max. 32 Zeichen gehalten, l�ngere
**          Namen werden abgeschnitten.
**          Ein OM_SET(OA_Name, NULL) bewirkt einen "Nameflush",
**          das hei�t der interne Zeichen-Puffer wird freigegeben,
**          allerdings kann er jederzeit wieder allokiert werden,
**          wenn ein OM_SET mit einem g�ltigen Namen ankommt.
**
**      OA_Class (G)  <Class *>
**          Enth�lt einen Pointer auf die interne Klassen-Struktur
**          (TrueClass) des Objects. Allerdings ist die privat,
**          Pointer darauf d�rfen nur zu Vergleichszwecken
**          herangezogen werden.
**
**      OA_ClassName (G) <UBYTE *>
**          Enth�lt einen Pointer auf den Namen der Klasse des
**          Objects. Dieser Name ist selbstverst�ndlich READ ONLY!
*/

/*-------------------------------------------------------------------
**  die ClassID der Nucleus-Class
*/
#define NUCLEUSCLASSID "nucleus.class"

/*-------------------------------------------------------------------
**  Die Definition der lokalen Instanz-Data der Nucleus-Class,
**  da die <nucleusclass> die RootClass von Nucleus darstellt,
**  erbt jedes beliebige Object diese Datenstruktur.
*/
struct nucleusdata {
    Class *o_Class;     /* Pointer auf Klasse dieses Objects */
    UBYTE *o_Name;      /* may be NULL */
};

/*-------------------------------------------------------------------
**  Konstanten
*/
#define SIZEOF_NAME (32)    /* Gr��e des Stringpuffers */

/*-------------------------------------------------------------------
**  Die von der NucleusClass direkt unterst�tzten Methoden
*/
#ifndef NUCLEUS_BOOPSI_FRIENDLY
    #define OM_BASE         0
    #define OM_NEW          (OM_BASE)
    #define OM_DISPOSE      (OM_BASE+1)
    #define OM_SET          (OM_BASE+2) /* Attribut setzen   */
    #define OM_GET          (OM_BASE+3) /* Attribut abfragen */
    #define OM_DUPLICATE    (OM_BASE+4) /* OBSOLETE */
    #define OM_NEWFROMIFF   (OM_BASE+5)
    #define OM_SAVETOIFF    (OM_BASE+6)
#else
    #define OM_BASE          0
    #define NOM_NEW          (OM_BASE)
    #define NOM_DISPOSE      (OM_BASE+1)
    #define NOM_SET          (OM_BASE+2) /* Attribut setzen   */
    #define NOM_GET          (OM_BASE+3) /* Attribut abfragen */
    #define NOM_DUPLICATE    (OM_BASE+4) /* OBSOLETE */
    #define NOM_NEWFROMIFF   (OM_BASE+5)
    #define NOM_SAVETOIFF    (OM_BASE+6)
#endif

/*-------------------------------------------------------------------
**  Attribute
*/
#define OMA_BASE        (TAG_USER)
#define OA_Name         (OMA_BASE)
#define OA_Class        (OMA_BASE+1)
#define OA_ClassName    (OMA_BASE+2)

/*-------------------------------------------------------------------
**  Die IFF-Definitionen f�r die Nucleus-Class
*/
#define OF_NUCLEUS  MAKE_ID ('R','O','O','T')
#define NUCIFF_Name MAKE_ID ('N','A','M','E')
/*
**  H�lt den [optionalen] Namen des Objects OHNE 0-Terminierung.
**  Die maximale L�nge des Chunks betr�gt 32 Zeichen.
**  Falls das Object namenlos ist, existiert auch kein
**  NUCIFF_Name-Chunk.
*/

/*-----------------------------------------------------------------*/
#endif


