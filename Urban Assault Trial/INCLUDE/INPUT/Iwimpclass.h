#ifndef INPUT_IWIMPCLASS_H
#define INPUT_IWIMPCLASS_H
/*
**  $Source: PRG:VFM/Include/input/iwimpclass.h,v $
**  $Revision: 38.4 $
**  $Date: 1996/11/25 18:31:43 $
**  $Locker:  $
**  $Author: floh $
**
**  Definition der iwimp.class, eine Input-Provider-Klasse,
**  welche die idev.class um einen "GUI-Zeigergerät" und
**  ein primitives Gadget-Input-Handling erweitert.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include "exec/types.h"
#endif

#ifndef EXEC_LISTS_H
#include "exec/lists.h"
#endif

#ifndef INPUT_IDEVCLASS_H
#include "input/idevclass.h"
#endif

#ifndef INPUT_CLICKBOX_H
#include "input/clickbox.h"
#endif

/*-------------------------------------------------------------------
**  NAME
**      iwimp.class -- GUI-Input-Klasse
**
**  FUNCTION
**      Erweitert die idev.class um eine Mauspointer-
**      Verwaltung (sowohl visuell als auch funktionell).
**      Die Mauspointer-Position kann gegen eine Liste
**      hierarchischer "ClickBoxen" getestet werden,
**      als Grundlage für ein primtives GUI-System.
**      Abgesehen davon ist ein iwimp.class Object
**      auch ganz nützlich in Fenster-Umgebungen,
**      es kann nämlich gefragt werden, ob es den
**      Input-Focus besitzt (wozu es tunlichst die
**      IWIMPA_Environment - Information benutzen sollte.
**
**  METHODS
**      IWIMPM_HASFOCUS
**          Msg:    ---
**          Ret:    ULONG [TRUE/FALSE]
**
**          Hiermit wird das Objekt gefragt, ob VFM momentan
**          den Input-Focus besitzt, also Eingabe-Information
**          ermitteln kann. In diesem Falle sollte es
**          mit TRUE antworten. Ansonsten sollte es FALSE
**          zurückgeben. Die iwimp.class gibt in dieser
**          Methode immer TRUE zurück, Subklassen
**          sollten die Methode überschreiben und das
**          IWIMPA_Environment Attribut benutzen, um zu
**          ermitteln, ob sie gerade den Input-Focus
**          haben. Das zentrale input.class Objekt wird
**          normalerweise nur einmal pro Frame das 
**          FocusWatcher-Objekt fragen, ob es den Input-Focus 
**          besitzt. Ist dies nicht der Fall, bricht die 
**          input.class sofort ab.
**
**      IWIMPM_ADDCLICKBOX
**          Msg:    struct iwimp_clickbox_msg
**          Ret:    void
**
**          Hängt die in der Msg übergeben ClickBox an die
**          interne ClickBox-Liste des Objekts. Man beachte
**          auch das Flags-Feld er Msg (siehe unten).
**
**      IWIMPM_REMCLICKBOX
**          Msg:    struct iwimp_clickbox_msg
**          Ret:    void
**
**          Entfernt die in der Msg übergebene ClickBox
**          aus der ClickBox-Liste des Objekts.
**
**      IWIMPM_GETCLICKINFO
**          Msg:    struct ClickInfo
**          Ret:    ---
**
**          Matcht die aktuelle "Zeigerposition" gegen die
**          ClickBox-Liste des Objekts und füllt die
**          ClickInfo-Struktur aus.
**
**          Anmerkung für Subklassen-Designer:
**          ----------------------------------
**          Folgende Felder müssen von der Subklasse bereitgestellt
**          werden:
**
**          Mauskoordinaten:
**          ClickInfo.act.scrx,scry
**          ClickInfo.down.scrx,scry
**          ClickInfo.up.scrx,scry
**
**          Mausbutton-Information:
**          ClickInfo.flags (CIF_MOUSEDOWN|CIF_MOUSEHOLD|CIF_MOUSEUP)
**
**          Das Matching gegen die ClickBox-Liste und das Ausfüllen
**          der restlichen ClickInfo-Struktur übernimmt dann
**          die iwimp.class selbst.
**
**  ATTRIBUTES
**
**      IWIMPA_Environment (ISG) [ULONG]
**          Übergibt dem Objekt systemspezischen
**          Environment Code, das kann z.B. einfach
**          nur ein VideoMode, oder sogar ein
**          Window-Pointer sein. Prinzipiell bekommt
**          ***jedes*** Input-Handler-Object vom zentralen
**          Input-Object das IWIMPA_Environment. Ob die
**          jeweilige Providerklasse allerdings das
**          Attribut auswertet, ist ihr überlassen.
**          Beim Design einer Subklasse sollte man allerdings
**          beachten, daß man sich von der Gfx-Engine abhängig
**          macht (diese stellt die Environment-ID bereit),
**          wenn man IWIMPA_Environment auswertet.
*/

/*-----------------------------------------------------------------*/
#define IWIMP_CLASSID   "iwimp.class"
/*-----------------------------------------------------------------*/
#define IWIMPM_BASE     (IDEVM_BASE+METHOD_DISTANCE)

#define IWIMPM_HASFOCUS     (IWIMPM_BASE)
#define IWIMPM_ADDCLICKBOX  (IWIMPM_BASE+1)
#define IWIMPM_REMCLICKBOX  (IWIMPM_BASE+2)
#define IWIMPM_GETCLICKINFO (IWIMPM_BASE+3)

/*-----------------------------------------------------------------*/
#define IWIMPA_BASE     (IDEVA_BASE+ATTRIB_DISTANCE)

#define IWIMPA_Environment  (IWIMPA_BASE) // (ISG) [ULONG]

/*-----------------------------------------------------------------*/
struct iwimp_data {
    ULONG flags;                // siehe unten
    struct MinList cbox_list;   // ClickBox-Liste
    struct ClickBox *hitbox_store;  // Pointer auf Req mit letzem Button-Click
    LONG hitbtn_store;          // Index des letzten angeclickten Buttons
};

/*-------------------------------------------------------------------
**  Message-Strukturen
*/
struct iwimp_clickbox_msg {
    struct ClickBox *cb;    // Pointer auf ClickBox-Struktur
    ULONG flags;            // siehe unten
};

#define IWIMPF_AddHead  (1<<0)  // an Anfang der CBox-Liste, sonst ans Ende

/*-----------------------------------------------------------------*/
#endif

