/*
**  $Source: PRG:VFM/Classes/_BaseClass/bc_family.c,v $
**  $Revision: 38.10 $
**  $Date: 1995/04/12 17:25:11 $
**  $Locker:  $
**  $Author: floh $
**
**  Methoden-Dispatcher zum Mother-Child-Handling.
**
**  (C) Copyright 1994 by A.Weissflog
*/
/* Amiga Includes */
#include <exec/types.h>

/* VFM Includes */
#include "types.h"
#include "nucleus/nucleus2.h"
#include "nucleus/nukedos.h"
#include "engine/engine.h"

#include "baseclass/baseclass.h"

/*-------------------------------------------------------------------
**  externe Referenzen
*/
_extern_use_nucleus
_extern_use_tform_engine

/*-----------------------------------------------------------------*/
_dispatcher(void, base_BSM_ADDCHILD, struct addchild_msg *msg)
/*
**  FUNCTION
**      Mit dieser Methode teilt man einem Object mit,
**      das übergebene Object in seine Child-Liste
**      aufzunehmen.
**
**  INPUTS
**      msg.child = Pointer auf Child, das angehängt werden soll
**                  [instance of base.class].
**
**  RESULTS
**      ---
**
**  CHANGED
**      14-Nov-94   floh    created
**      17-Nov-94   floh    serial debug code
**      19-Jan-95   floh    Nucleus2-Revision
*/
{
    struct base_data *bd = INST_DATA(cl,o);

    /* die Sache ist easy: einfach ein BSM_NEWMOTHER Methode  */
    /* mit ein paar Informationen auf das neue Child anwenden */
    struct newmother_msg out_msg;

    /* Pointer auf "mich" und auf "meine" ChildList */
    out_msg.mother = o;
    out_msg.child_list = &(bd->ChildList);

    /* Pointer auf TForm nur, wenn "ich" eine geometrische */
    /* Existenz bin (also ein Skeleton existiert) */
    if (bd->Skeleton)   out_msg.mother_tform = &(bd->TForm);
    else                out_msg.mother_tform = NULL;

    /* und ein BSM_NEWMOTHER auf mein neues Child */
    _methoda(msg->child, BSM_NEWMOTHER, (Msg *) &out_msg);

    /* das war's schon */
}

/*-----------------------------------------------------------------*/
_dispatcher(void, base_BSM_NEWMOTHER, struct newmother_msg *msg)
/*
**  FUNCTION
**      Unternimmt alle Aktionen, die notwendig sind, um Child
**      einer neuen Mother zu werden. Normalerweise wird BSM_NEWMOTHER
**      von der Mother selbst innerhalb eines BSM_ADDCHILD an
**      ihr neues Kind geschickt.
**      Im einzelnen werden folgende Aktionen unternommen:
**          (1) Falls das Object bereits eine Mother hat, entfernt
**              es sich zuerst aus deren ChildList.
**          (2) Der in <msg.mother> übergebene Pointer wird in die
**              eigene LID nach <base_data.Mother> eingetragen
**              (es ist ok, ein <msg.mother = NULL> zu übergeben!
**          (3) Falls der <msg.child_list> Pointer ok ist, hängt sich das
**              Object in diese Liste (mittels der eigenen ChildNode).
**          (4) Der in <msg.mother_tform> übergebene Pointer wird in die
**              eigene TForm in das Feld <tform.mother> eingetragen.
**              <msg.mother_tform = NULL> ist ok, falls das Object
**              nicht im Koordinaten-System seiner Mother, sondern
**              direkt im Global-System definiert sein soll.
**          (5) Falls das Object ein MainChild ist, unternimmt es alles
**              Notwendige, damit der "Main-Zweig" im "Welten-Baum"
**              mitwandert.
**
**      Um ein Object von seiner Mother abzutrennen MUSS man folgende
**      Message übergeben:
**          msg.mother       = NULL;
**          msg.mother_tform = NULL;
**          msg.child_list   = NULL;
**
**  INPUTS
**      siehe FUNCTION
**
**  RESULTS
**      ---
**
**  CHANGED
**      14-Nov-94   floh    created
**      17-Nov-94   floh    serial debug code
**      21-Dec-94   floh    BSM_MAIN hat eine veränderte Msg bekommen,
**                          dahingehend angepaßt
**      19-Jan-95   floh    Nucleus2-Revision
**      12-Apr-95   floh    (1) klinkt sich nicht mehr mit _AddTail()
**                              ein, sondern mit _AddHead().
**                          (2) ...und gleich wieder rückgängig gemacht,
**                              sonst hätte sich fatalerweise bei jedem
**                              Save/Load die Reihenfolge der Children
**                              umgekehrt.
*/
{
    struct base_data *bd = INST_DATA(cl,o);

    struct MinNode *my_child_node = (struct MinNode *) &(bd->ChildNode);
    ULONG main_child              = (bd->Flags) & BSF_MainChild;
    struct main_msg mchild_msg;

    /* Bin ich Child einer Mother? */
    if (bd->Mother) {

        /* ...dann aus ChildList meiner alten Mother entfernen */
        _Remove((struct Node *)my_child_node);

        /* falls ich ein MainChild bin, muß ich meiner Mother */
        /* mitteilen, daß ich sie verlasse... */
        if (main_child) {
            mchild_msg.main_child  = (Object *) -1L;
            mchild_msg.main_object = NULL;
            _methoda(bd->Mother, BSM_MAIN, (Msg *) &mchild_msg);
        };
    };

    /* Übernehme neue Mother (may be NULL) */
    if (bd->Mother = msg->mother) {

        /* in ChildList der (optionalen) Mother einklinken */
        _AddTail((struct List *)msg->child_list, (struct Node *)my_child_node);

        /* falls ich ein MainChild bin, muß ich das meiner */
        /* neuen Mother mitteilen */
        if (main_child) {
            mchild_msg.main_child  = o;
            mchild_msg.main_object = o;
            _methoda(bd->Mother, BSM_MAIN, (Msg *) &mchild_msg);
        };
    };

    /* Mother's TForm als eigene TForm-Mother (may be NULL) */
    /* (für die Transformations-Hierarchie)   */
    bd->TForm.mother = msg->mother_tform;

    /* das war's schon! */
}

/*-----------------------------------------------------------------*/
_dispatcher(void, base_BSM_MAIN, struct main_msg *msg)
/*
**  FUNCTION
**      Interne Lowlevel-Methode zum MainChild-Handling.
**
**      Folgende "Spezial-Werte" sind für <main_msg.main_child> definiert:
**
**          0L  -> Object, auf das BSM_MAIN angewendet wurde,
**                 ist das MainObject. Dieses Object wird an seine
**                 Mother die Methode BSM_MAIN mit einem Pointer
**                 auf sich selbst schicken, damit wird
**                 der Main-Zweig bis hinauf zum Welt-Object aufgebaut.
**                 <main_msg.main_object = NULL>
**          Object * -> internal use only (see above)...
**                      <main_msg.main_object = MainObject>
**          -1L -> wenn ein MainChild seine Mother verläßt (siehe
**                 BSM_ADDCHILD), schickt es ein
**                 DoMethod(mother, BSM_MAIN, -1L) an seine Mother.
**                 Diese löscht ihren MainChild-Pointer und schickt
**                 die Methode wiederum an ihre Mother. Damit wandert
**                 der gesamte Main-Zweig automatisch mit dem
**                 MainObject mit.
**                 <main_msg.main_object = NULL>
**
**      Sobald ein Object die Methode BSM_MAIN bekommt, setzt es
**      bei einem <msg.main_child != -1> das interne BSF_MainChild-Flag.
**      Andernfalls wird das Flag gelöscht. Nur zur Information ;-)
**
**  INPUTS
**      Hier nochmal die zugelassenen Struktur-Inhalte:
**          <main_child>=NULL -> <main_object>=NULL
**          <main_child>=MainChild -> <main_object>=MainObject
**          <main_chidl>=-1L  -> <main_object>=NULL
**
**  RESULTS
**      ---
**
**  CHANGED
**      19-Nov-94   floh    created
**      12-Dec-94   floh    Das MainObject (also das Ende am Ende
**                          des Main-Zweiges) bekommt jetzt das Flag
**                          BSF_MainObject gesetzt (gleichzeitig mit
**                          BSF_MainChild).
**      21-Dec-94   floh    BSM_MAIN ist jetzt eine interne Methode.
**                          Applikationen müssen BSM_SETMAINOBJECT
**                          benutzen, welches das vorherige MainObject
**                          automatisch "abmeldet", so daß der Main-Zweig
**                          intakt bleibt.
**      19-Jan-95   floh    Nucleus2-Revision
**
**  NOTE
**      Falls die Methode den Main-Zweig hinaufgereicht werden muß,
**      wird dazu gleich die "originale" Msg überschrieben!
*/
{
    struct base_data *bd = INST_DATA(cl,o);

    /* irgendein "Sonderfall"? */
    switch ((ULONG) msg->main_child) {
        case 0L:
            /*** ich werde neues Main-Object... ***/
            bd->MainChild  = NULL;
            bd->MainObject = o;     /* das bin ich! */
            bd->Flags    |= (BSF_MainChild|BSF_MainObject);

            /* neuen Main-Zweig aufbauen */
            if (bd->Mother) {
                msg->main_child  = o;
                msg->main_object = o;
                _methoda(bd->Mother, BSM_MAIN, (Msg *) msg);
            };

            /* Ich werde auch neuer Viewer! */
            _SetViewer(&(bd->TForm));
            break;

        case -1L:
            /*** ich verliere gerade meinen MainChild-Status ***/
            bd->MainChild  = NULL;
            bd->MainObject = NULL;
            bd->Flags    &= ~(BSF_MainChild|BSF_MainObject);

            /* alten Main-Zweig abbauen */
            if (bd->Mother) _methoda(bd->Mother, BSM_MAIN, (Msg *) msg);
            break;

        default:
            /*** ich bin irgendwo mittem im neuen Main-Zweig... ***/
            bd->MainChild  = msg->main_child;
            bd->MainObject = msg->main_object;
            bd->Flags    |= BSF_MainChild;
            bd->Flags    &= ~BSF_MainObject;

            /* neuen Main-Zweig weiter aufbauen */
            if (bd->Mother) {
                msg->main_child = o;
                _methoda(bd->Mother, BSM_MAIN, (Msg *) msg);
            };
            break;
    };

    /* das dürfte es schon gewesen sein... */
}

/*-----------------------------------------------------------------*/
_dispatcher(Object *, base_BSM_SETMAINOBJECT, struct setmainobject_msg *msg)
/*
**  FUNCTION
**      Öffentliche Methode zum Setzen des MainChilds (sprich
**      des Viewer-Objects). Die Methode muß mit einem Pointer
**      auf das neue MainObject auf das ROOT(!)-Object der
**      Hierarchie angewendet werden. Dadurch ist sichergestellt,
**      das das vorherige Main-Object vorher korrekt abgemeldet
**      wird.
**      DAS MAIN-OBJECT MUSS CHILD BZW. SUB..CHILD DES ROOT-OBJECTS
**      SEIN (logischerweise).
**
**      Beispiel:
**          Object *old_main = DoMethod(root, BSM_SETMAINOBJECT, new_main)
**
**  INPUTS
**      msg -> msg.main_main_object = Pointer auf neues Main-Object
**                                    darf NULL sein, wenn KEIN Main-Object
**                                    vorhanden sein soll.
**
**  RESULTS
**      Object * -> Pointer auf vorheriges Main-Object, oder
**                  NULL, falls keins existierte.
**
**  CHANGED
**      21-Dec-94   floh    created
**      19-Jan-95   floh    Nucleus2-Revision
*/
{
    Object *old;
    struct main_msg mainmsg;
    struct base_data *bd = INST_DATA(cl,o);

    /* alten Main-Zweig abbauen */
    old = bd->MainObject;
    if (old) {
        mainmsg.main_child  = (Object *) -1L;
        mainmsg.main_object = NULL;
        _methoda(old,BSM_MAIN,(Msg *)&mainmsg);
    };

    /* neuen Main-Zweig aufbauen [optional] */
    if (msg->main_object) {
        mainmsg.main_child  = NULL;
        mainmsg.main_object = NULL;
        _methoda(msg->main_object,BSM_MAIN,(Msg *)&mainmsg);
    };

    /* das war's */
    return(old);

}

