/*
**  $Source: PRG:VFM/Classes/_BaseClass/bc_trigger.c,v $
**  $Revision: 38.32 $
**  $Date: 1998/01/06 14:43:49 $
**  $Locker: floh $
**  $Author: floh $
**
**  Dieses Modul enthält den BSM_TRIGGER-Dispatcher, sowie
**  alle "Subdispatcher", die von diesem aufgerufen werden,
**  also BSM_MOTION, BSM_DOCOLLISION, BSM_CHECKCOLLISION,
**  BSM_HANDLECOLLISION und BSM_PUBLISH.
**
**  (C) Copyright 1994 by A.Weissflog
*/
/*** Amiga Includes ***/
#include <exec/types.h>
#include <exec/lists.h>
#include <exec/nodes.h>

/*** ANSI Includes ***/
#include <stdlib.h>

/*** VFM Includes ***/
#include "nucleus/nucleus2.h"
#include "engine/engine.h"
#include "visualstuff/ov_engine.h"

#include "baseclass/baseclass.h"
#include "ade/ade_class.h"
#include "skeleton/skeletonclass.h"
#include "bitmap/displayclass.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine
_extern_use_ov_engine
_extern_use_input_engine

/*===================================================================
**  Der "Publish Stack" und der "Argument Stack". Beide sind
**  ungefähr so groß, um durchschnittlich 1000 Einträge
**  aufzunehmen.
*/
#define STACK_NUM_ENTRIES   2100
#define ARGSTACK_ENTRY_SIZE 128     /* ca. 96 Bytes pro Arg-Stack Entry*/

#ifdef AMIGA
__far struct pubstack_entry PubStack[STACK_NUM_ENTRIES];
__far UBYTE ArgStack[STACK_NUM_ENTRIES * ARGSTACK_ENTRY_SIZE];
#else
struct pubstack_entry PubStack[STACK_NUM_ENTRIES];
UBYTE ArgStack[STACK_NUM_ENTRIES * ARGSTACK_ENTRY_SIZE];
#endif

/* für Stack-Overflow Checks: */
UBYTE *EndOf_ArgStack = ArgStack + sizeof(ArgStack) - 256;
struct pubstack_entry *EndOf_PubStack = PubStack + STACK_NUM_ENTRIES;

/*=================================================================*/

/*-----------------------------------------------------------------*/
_dispatcher(void, base_BSM_MOTION, struct trigger_msg *msg)
/*
**  FUNCTION
**      Handelt Bewegung/Rotation ab und berechnet ein paar
**      Parameter für die (evtl.) nachfolgende Kollisions-Kontrolle.
**      (für mehr Info siehe "baseclass/baseclass.h"
**
**      Diese Methode sollte nie direkt auf ein Object angewendet
**      werden! Der normale Weg geht über den BSM_TRIGGER-Dispatcher!
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      01-Dec-94   floh    created
**      03-Dec-94   floh    -> neues TForm-Format, alles angepaßt...
**      04-Dec-94   floh    BSM_MOTION wurde nicht an die Kinder
**                          weitergegeben
**      12-Dec-94   floh    (1) Bewegung und Rotation nur noch, wenn
**                              Object geometrische Existenz ist, Sensor-Pool
**                              Erzeugung wurde nach BSM_DOCOLLISION()
**                              ausgelagert.
**                          (2) Falls BSF_HandleInput gesetzt ist, wird
**                              jetzt BSM_HANDLEINPUT invoked.
**      22-Dec-94   floh    Vorzeichen-Fehler bei Negativ-Rotation
**                          entfernt.
**      20-Jan-95   floh    Nucleus2-Revision
**      03-Mar-95   floh    Aaargh, ich Laie. Diese mysteriösen Abstürze
**                          bei negativer Drehung kamen daher, weil die
**                          "Vorzeichen-Korrektur" nicht funktioniert hat
**                          (<<16 vergessen).
**      13-May-95   floh    _TFormToGlobal() auf die Object-eigene
**                          TForm wird jetzt inerhalb BSM_PUBLISH
**                          erledigt!!! Das heißt, die globalen
**                          Parameter einer TForm sind erst nach einem
**                          BSM_PUBLISH aktuell!!!
**      15-May-95   floh    Unterstützung für multiples Publishing
**                          innerhalb eines einzelnen Frames: Falls die
**                          GlobalTime in der trigger_msg mit dem TimeStamp
**                          in der LID übereinstimmt, wird weder etwas
**                          bewegt, noch BSM_HANDLEINPUT invoked.
**      24-Sep-95   floh    alte Position wird nicht mehr aufgehobe,
**                          weil der Kollisions-Komplex gestorben ist
*/
{
    struct base_data *bd = INST_DATA(cl,o);
    struct MinNode *nd;

    /* neuer Frame? */
    if (msg->global_time != bd->TimeStamp) {

        /* JA */
        bd->TimeStamp = msg->global_time;

        /* Object "geometrische Existenz"? */
        if (bd->Skeleton) {

            /* HandleInput-Flags gesetzt? */
            if (bd->Flags & BSF_HandleInput) {
                /* dann ... */
                _methoda(o,BSM_HANDLEINPUT,(Msg *)msg);
            };

            /* lineare Bewegung nur, falls BSF_MoveMe */
            if (bd->Flags & BSF_MoveMe) {

                /* XXX */
                FLOAT flt_time = (FLOAT) msg->frame_time;

                /* [loc] = [loc] + ([vec]*frame_time) */
                bd->TForm.loc.x += bd->TForm.vec.x*flt_time;
                bd->TForm.loc.y += bd->TForm.vec.y*flt_time;
                bd->TForm.loc.z += bd->TForm.vec.z*flt_time;
            };

            /* Rotation nur, falls BSF_RotateMe */
            if (bd->Flags & BSF_RotateMe) {

                register LONG ax;
                register LONG ay;
                register LONG az;
                WORD fix_time = (WORD) msg->frame_time;

                /* [ax,ay,az] = [ax,ay,az] + ([rx,ry,rz]*frame_time) */
                ax = (bd->TForm.ang.x + bd->TForm.rot.x*fix_time) % (359<<16);
                if (ax < 0)  ax += 360<<16;  /* Vorzeichen-Korrektur */
                bd->TForm.ang.x = ax;
                ay = (bd->TForm.ang.y + bd->TForm.rot.y*fix_time) % (359<<16);
                if (ay < 0)  ay += 360<<16;  /* Vorzeichen-Korrektur */
                bd->TForm.ang.y = ay;
                az = (bd->TForm.ang.z + bd->TForm.rot.z*fix_time) % (359<<16);
                if (az < 0)  az += 360<<16;  /* Vorzeichen-Korrektur */
                bd->TForm.ang.z = az;

                /* weil an den Winkeln rumgefummelt wurde, muß die */
                /* lokale Rot/Scale-Matrix neu berechnet werden    */
                _RefreshTForm(&(bd->TForm));
            };
        }; /* if (exists Skeleton) */
    }; /* if (new frame) */

    /* BSM_MOTION an alle Children verteilen... */
    for (nd=bd->ChildList.mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {
        _methoda(((struct ObjectNode *)nd)->Object, BSM_MOTION, (Msg *)msg);
    };
}

/*-----------------------------------------------------------------*/
_dispatcher(void, base_BSM_HANDLEINPUT, struct trigger_msg *msg)
/*
**  FUNCTION
**      Wertet Input-Informationen aus (via (struct VFMInput *) in
**      trigger_msg) und ändert abhängig davon Object-Attribute.
**      (bitte beachten, daß BSM_HANDLEINPUT ausschließlich
**      aus BSM_MOTION aufgerufen wird).
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      12-Dec-94   floh    erstmal leer...
**      20-Jan-95   floh    Nucleus2-Revision
**      03-Mar-95   floh    endlich mal mit was sinnvollem ausgefüllt.
**      26-Feb-96   floh    schnell angepaßt an neue VFMInput Struktur
**      28-Feb-96   floh    sorgfältiger angepaßt :-)
*/
{
    struct base_data *bd = INST_DATA(cl,o);
    tform *tf = &(bd->TForm);

    ULONG buttons;

    /* Buttons holen */
    buttons  = msg->input->Buttons;

    bd->Flags |= (BSF_MoveMe|BSF_RotateMe);

    /*** Button 0: zentrieren ***/
    if (buttons & SYS_Button_Center) {
        tf->ang.x = 0;
        tf->ang.y = 0;
        tf->ang.z = 0;
    };

    /*** xyz-Rotation ***/
    tf->rot.x = (LONG) (-msg->input->Slider[SYS_Slider_RotX] * 16384.0);
    tf->rot.y = (LONG) (-msg->input->Slider[SYS_Slider_RotY] * 16384.0);
    tf->rot.z = (LONG) (-msg->input->Slider[SYS_Slider_RotZ] * 16384.0);

    /*** Walking ***/
    tf->vec.x = tf->loc_m.m31 * -msg->input->Slider[SYS_Slider_Walk];
    tf->vec.y = 0;
    tf->vec.z = tf->loc_m.m33 * -msg->input->Slider[SYS_Slider_Walk];

    /*** Flying ***/
    tf->vec.x += tf->loc_m.m31 * -msg->input->Slider[SYS_Slider_Fly];
    tf->vec.y += tf->loc_m.m32 * -msg->input->Slider[SYS_Slider_Fly];
    tf->vec.z += tf->loc_m.m33 * -msg->input->Slider[SYS_Slider_Fly];
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, base_BSM_PUBLISH, struct basepublish_msg *msg)
/*
**  FUNCTION
**      Falls sichtbar (dies wird durch diverse voreingestellte
**      und online ermittelte Parameter bestimmt), führe
**      die Koordinaten-Transformation durch und wende
**      an jedes ADE die Methode ADEM_PUBLISH an, falls
**      BSA_PublishAll gesetzt, leite BSM_PUBLISH an alle
**      Kinder weiter, andernseits nur an das MainChild.
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      05-Dec-94   floh    created
**      12-Dec-94   floh    Ermittlung, ob MainObject oder nicht, ist
**                          jetzt etwas effizienter (via MainObject-Flag).
**      20-Jan-95   floh    (1) Nucleus2-Revision
**                          (2) Neue Msg-Struktur, füllt jetzt außerdem
**                              indirekt über ADEM_PUBLISH die neuen
**                              ArgStack und PublishStack auf.
**      02-Mar-95   floh    Für jedes ADE, auf das ADEM_PUBLISH angewendet
**                          wurde, wird msg->ade_count inkrementiert.
**                          BSM_TRIGGER überträgt dann diesen Wert als
**                          Feedback in die trigger_msg.
**      16-Mar-95   floh    (1) ADE-Count-Feedback leicht geändert (wird
**                              jetzt letztendlich vom ADE vorgenommen,
**                              -> wegen komplexen ADEs (amesh.class ...)
**                          (2) BSF_PublishAll wird jetzt ignoriert,
**                              BSM_PUBLISH wird generell an alle
**                              Children weitergegeben.
**      09-May-95   floh    Die Sensor-Pool-Ermittlung samt Sichtbarkeits-
**                          Check findet jetzt hier statt. Zur Zeit
**                          wird auf alle Fälle ein BSM_PUBLISH auf die
**                          Children angewendet, egal, ob die Mother sichtbar
**                          ist, oder nicht. Sobald ein Kind sichtbar ist,
**                          wird der Return-Value TRUE. Ein zukünftiges
**                          Attribut BSA_Container wird diese Sache
**                          Optimieren (Children ignorieren, sobald Mother
**                          nicht sichtbar).
**      10-May-95   floh    Test auf Publish Stack Overflow jetzt per
**                          msg->eoargstack.
**      12-May-95   floh    RetVal BOOL brachte einige Probleme (16 Bit
**                          Rückgabe-Wert). Ist jetzt ULONG.
**      02-Jul-95   floh    _SensorToGlobal() ersetzt durch _LocalToGlobal(),
**                          d.h. keine automatische B-Box-Erzeugung mehr.
**      07-Jul-95   floh    neues Feld in <publish_msg> namens <time_stamp>
**                          wird jetzt neu ausgefüllt.
**      12-Sep-95   floh    übernimmt jetzt owner_id aus <basepublish_msg>
**                          nach <publish_msg>, außerdem einen Pointer
**                          auf die eingebettete TForm-Struktur
**      24-Sep-95   floh    no more collisions
**      09-Nov-95   floh    es gibt jetzt 2 PubStacks, Front-To-Back und
**                          Back-To-Front.
**      12-Nov-95   floh    es gibt jetzt wieder nur einen PubStack,
**                          das war ein klassischer Denkfehler...
**      17-Jan-96   floh    massiv revised & updated.
**                          + Debugging
**      11-Jun-96   floh    + Änderungen in der <publish_msg> für die
**                            ADEs.
**      06-Apr-97   floh    + Polygon-Overkill brachte Engine zum Absturz
**      01-Jun-97   floh    + neue <min_z> und <max_z> Werte in
**                            <basepublish_msg>
*/
{
    struct base_data *bd = INST_DATA(cl,o);
    struct MinNode *nd;
    BOOL vis = FALSE;

    /** das Ganze geht nur, wenn geometrische Existenz, **/
    if (bd->Skeleton) {

        /** Darstellung hat nur Sinn, wenn Object NICHT das Main-Object ist **/
        if (!(bd->Flags & BSF_MainObject)) {

            struct local2vwr_msg l2v_msg;

            /*** TForm globalisieren ***/
            _TFormToGlobal(&(bd->TForm));

            /*** Skeleton nach Viewer-Space transformieren ***/
            l2v_msg.local  = &(bd->TForm);
            l2v_msg.viewer = _GetViewer();
            l2v_msg.min_z  = msg->min_z;
            l2v_msg.max_z  = msg->max_z;
            if (vis = _methoda(bd->Skeleton, SKLM_LOCAL2VWR, &l2v_msg)) {

                /*** SICHTBAR, ADEs publishen ***/

                /* ADE-Publish-Msg komplettieren */
                bd->PubMsg.owner_id   = msg->owner_id;
                bd->PubMsg.pubstack   = msg->pubstack;
                bd->PubMsg.argstack   = msg->argstack;
                bd->PubMsg.viewer     = l2v_msg.viewer;
                bd->PubMsg.owner      = l2v_msg.local;
                bd->PubMsg.min_z      = msg->min_z;
                bd->PubMsg.max_z      = msg->max_z;
                bd->PubMsg.time_stamp = msg->global_time;
                bd->PubMsg.frame_time = msg->frame_time;
                bd->PubMsg.ade_count  = 0;
                bd->PubMsg.sklto      = bd->Skeleton;

                /*** ADEM_PUBLISH auf alle ADEs anwenden... ***/
                for (nd=bd->ADElist.mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {

                    /* Stack-Overflow verhindern... */
                    if ((bd->PubMsg.argstack < (msg->eoargstack)) &&
                       ((bd->PubMsg.ade_count+msg->ade_count)<msg->maxnum_ade))
                    {
                        /* ADEM_PUBLISH auf nächstes ADE */
                        _methoda(((struct ObjectNode *)nd)->Object,
                                   ADEM_PUBLISH,
                                   &(bd->PubMsg));

                    } else break; /*** STACK OVERFLOW ***/
                };
                /*** Feedback zurück nach <msg> ***/
                msg->ade_count += bd->PubMsg.ade_count;
                msg->pubstack   = bd->PubMsg.pubstack;
                msg->argstack   = bd->PubMsg.argstack;
            };
        };
    };

    /* BSM_PUBLISH rekursiv nach unten weitergeben... */
    for (nd=bd->ChildList.mlh_Head;nd->mln_Succ;nd=nd->mln_Succ) {

        /*** sichtbare Childs machen Gesamt-Sichtbarkeits-Status TRUE ***/
        Object *child = ((struct ObjectNode *)nd)->Object;
        if (_methoda(child,BSM_PUBLISH,msg)) vis=TRUE;
    };

    /*** Ende ***/
    return(vis);
}

/*-------------------------------------------------------------------
**  Compare Function für qsort() auf Publish Stack.
*/
int __CDECL pubstack_cmp(struct pubstack_entry *first, struct pubstack_entry *second)
{
    return(first->depth - second->depth);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, base_BSM_TRIGGER, struct trigger_msg *msg)
/*
**  FUNCTION
**      Die mit Abstand mächtigste Methode der base.class.
**      Die BSM_TRIGGER Methode definiert den Herzschlag eines
**      base.class Objects. Sie ist das einzige zyklisch
**      wiederkehrende Ereignis und wird einmal pro Frame
**      auf das Object angewendet. Die BSM_TRIGGER Methode
**      ist die einzige Gelegenheit, solche Sachen wie
**      Bewegung, Darstellung, Jobhandling etc. abzuhandeln.
**
**      Um die Welt von der Framerate abzukoppeln, wird in der
**      >trigger_msg< die aktuelle Absolut-Zeit seit Beginn
**      der Existenz der Welt und die Zeit-Differenz zum
**      letzten Frame in >Ticks< mitgegeben. Sämtliche
**      kontinuierliche Prozesse werden mit diesen Werten
**      synchronisiert.
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      05-Dec-94   floh    created
**      20-Jan-95   floh    (1) Nucleus2-Revision
**                          (2) enthält jetzt die komplette Abhandlung eines
**                              Frames
**      22-Jan-95   floh    (1) debugging...
**                          (2) auf dem Amiga wurde die "DrawFunc()" nicht
**                              mit dem Parameter in __a0 aufgerufen
**                              (falsches Prototyping).
**      02-Mar-95   floh    Benchmark-Feedback: Für jedes bearbeitete
**                          ADE wird msg->ade_count inkrementiert (indirekt)
**                          via BSM_PUBLISH, für jedes tatsächlich
**                          gezeichnete ADE wird msg->ade_drawn
**                          inkrementiert. Die beiden Zähler werden
**                          NICHT mit 0 initialisiert, damit kann man
**                          auch über mehrere Frames zählen.
**      10-May-95   floh    Beim Init der basepublish_msg wird jetzt das
**                          neue Feld <eoargstack> (für EndOfArgStack)
**                          korrekt ausgefüllt.
**      13-May-95   floh    Am Anfang des Frames wird die jetzt TForm des
**                          Viewers globalisiert.
**      12-Sep-95   floh    in die bpub_msg wird jetzt die <owner_id>
**                          aus base_data.Id übernommen.
**      24-Sep-95   floh    no more collisions
**      30-Sep-95   floh    vor _ToggleDisplays() wird jetzt das neue
**                          _FlushGfx() aufgerufen.
**      09-Nov-95   floh    es gibt jetzt zwei PubStacks, einer der von
**                          vorn nach hinten sortiert wird und einer,
**                          der von hinten nach vorn sortiert wird...
**      12-Nov-95   floh    wieder nur ein PubStack
*/
{
    struct base_data *bd = INST_DATA(cl,o);
    struct basepublish_msg bpub_msg;
    ULONG num_elm, act_elm;
    tform *vwr;
    Object *gfxo;

    /*** TForm des aktuellen Viewers globalisieren ***/
    if (vwr = _GetViewer()) _TFormToGlobal(vwr);

    /*** Bewegung ***/
    _methoda(o,BSM_MOTION,(Msg *)msg);

    /*** BSM_PUBLISH -> PubStack und ArgStack aufbauen ***/
    bpub_msg.frame_time   = msg->frame_time;
    bpub_msg.global_time  = msg->global_time;
    bpub_msg.pubstack     = PubStack;
    bpub_msg.argstack     = (struct argstack_entry *) ArgStack;
    bpub_msg.eoargstack   = (struct argstack_entry *) EndOf_ArgStack;
    bpub_msg.ade_count    = msg->ade_count;
    bpub_msg.maxnum_ade   = 1000;
    bpub_msg.owner_id     = bd->Id;

    _methoda(o,BSM_PUBLISH,(Msg *)&bpub_msg);

    msg->ade_count = bpub_msg.ade_count;

    /*** PubStack abhandeln ***/
    num_elm = (bpub_msg.pubstack - PubStack);
    msg->ade_drawn += num_elm;
    if (num_elm > 1) {
        qsort(PubStack, num_elm, sizeof(struct pubstack_entry), pubstack_cmp);
    };

    /*** Frame abhandeln ***/
    _OVE_GetAttrs(OVET_Object,&gfxo,TAG_DONE);
    _methoda(gfxo,DISPM_Begin,NULL);
    _set(gfxo,RASTA_BGPen,0);
    _methoda(gfxo,RASTM_Clear,NULL);

    /* alle Elemente auf dem Front-To-Back-Pubstack darstellen */
    _methoda(gfxo,RASTM_Begin3D,NULL);
    for (act_elm=0; act_elm<num_elm; act_elm++) {

        /* Pointer auf Args-Block */
        void *drawargs = PubStack[act_elm].args + 1;

        /* ermittle Pointer auf DrawFunc() */
        #ifdef AMIGA
            void __asm (*drawfunc) (__a0 void *);
            drawfunc = (void __asm (*) (__a0 void *))
                       PubStack[act_elm].args->draw_func;
        #else
            void (*drawfunc) (void *);
            drawfunc = PubStack[act_elm].args->draw_func;
        #endif

        /* DrawFunc() mit Pointer auf Args-Block aufrufen */
        drawfunc(drawargs);
    };

    _methoda(gfxo,RASTM_End3D,NULL);
    _methoda(gfxo,DISPM_End,NULL);

    /* das war's schon */
}

