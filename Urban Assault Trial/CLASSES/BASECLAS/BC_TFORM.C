/*
**  $Source: PRG:VFM/Classes/_BaseClass/bc_tform.c,v $
**  $Revision: 38.8 $
**  $Date: 1995/02/14 20:48:33 $
**  $Locker:  $
**  $Author: floh $
**
**  Methoden-Dispatcher der base.class, die die eingebettete
**  TForm-Struktur direkt manipulieren.
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
_dispatcher(void, base_BSM_POSITION, struct flt_vector_msg *msg)
/*
**  FUNCTION
**      Setzt die Position [loc] des Objects. In der <mask>
**      der vector_msg wird angegeben, welche Koordinaten
**      ignoriert und welche gesetzt werden sollen:
**
**      DoMethod(o,BSM_POSITION,(VEC_X|VEC_Y),100.0,200.0,0.0);
**
**      Bitte beachten, daß die numerischen Werte als FLOAT's
**      erwartet werden!
**
**      BITTE UNBEDINGT DEN TEXT ZUR <struct vector_msg> DURCHLESEN!
**      (in "baseclass/baseclass.h")
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      12-Nov-94   floh    created
**      17-Nov-94   floh    serial debug code
**      28-Nov-94   floh    jetzt mit flt_vector_msg
**      03-Dec-94   floh    die TForm hat jetzt eine strukturiertere
**                          Struktur, alles angepaßt...
**      20-Jan-94   floh    Nucleus2-Revision
*/
{
    /* Pointer auf TForm... */
    register tform *tf = &(((struct base_data *)INST_DATA(cl,o))->TForm);

    /* Maske aus vector_msg holen */
    register ULONG mask = msg->mask;

    /* Position absolut oder relativ beeinflussen? */
    if (mask & VEC_RELATIVE) {

        /* welche Koordinaten RELATIV modifizieren? */
        if (mask & VEC_X)   tf->loc.x += msg->x;
        if (mask & VEC_Y)   tf->loc.y += msg->y;
        if (mask & VEC_Z)   tf->loc.z += msg->z;

    } else {

        /* welche Koordinaten ABSOLUT modifizieren? */
        if (mask & VEC_X)   tf->loc.x = msg->x;
        if (mask & VEC_Y)   tf->loc.y = msg->y;
        if (mask & VEC_Z)   tf->loc.z = msg->z;
    };
}

/*-----------------------------------------------------------------*/
_dispatcher(void, base_BSM_VECTOR, struct flt_vector_msg *msg)
/*
**  FUNCTION
**      Setzt den Bewegungs-Vektor [vec] des Objects. In der
**      <mask> der vector_msg wird angegeben, welche Raum-Dimensionen
**      ignoriert bzw. modifiziert werden sollen:
**
**      DoMethod(o,BSM_VECTOR,(VEC_X|VEC_Z),0.46,0.0,1.23);
**
**      Bitte beachten, daß die numerischen Werte als FLOAT's
**      erwartet werden!
**      Falls das <mask>-Bit VEC_FORCE_ZERO gesetzt ist, wird die
**      Bewegung vollständig ausgeschaltet, es findet dann auch
**      keine aktive Kollisions-Kontrolle mehr statt
**      (via BSF_MoveMe-Flag).
**
**      BITTE UNBEDINGT DEN TEXT ZUR <struct vector_msg> DURCHLESEN!
**      (in "baseclass/baseclass.h")
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      12-Nov-94   floh    created
**      17-Nov-94   floh    serial debug code
**      28-Nov-94   floh    jetzt mit flt_vector_msg
**      03-Dec-94   floh    die TForm hat jetzt eine strukturiertere
**                          Struktur, alles angepaßt...
**      20-Jan-95   floh    Nucleus2-Revision
*/
{
    /* Pointer auf LID */
    register struct base_data *bd = INST_DATA(cl,o);

    /* Maske aus vector_msg holen */
    register ULONG mask = msg->mask;

    /* Sonderfall: Bewegung ausschalten? */
    if (mask & VEC_FORCE_ZERO) {

        /* dann BSF_MoveMe-Flag löschen */
        bd->Flags &= ~BSF_MoveMe;

    } else {

        /* Pointer auf TForm */
        register tform *tf = &(bd->TForm);

        /* BSF_MoveMe-Flag setzen */
        bd->Flags |= BSF_MoveMe;

        /* relativ oder absolut modifizieren? */
        if (mask & VEC_RELATIVE) {

            /* welche Koordinaten RELATIV modifizieren? */
            if (mask & VEC_X)   tf->vec.x += msg->x;
            if (mask & VEC_Y)   tf->vec.y += msg->y;
            if (mask & VEC_Z)   tf->vec.z += msg->z;

        } else {

            /* welche Koordinaten ABSOLUT modifizieren? */
            if (mask & VEC_X)   tf->vec.x = msg->x;
            if (mask & VEC_Y)   tf->vec.y = msg->y;
            if (mask & VEC_Z)   tf->vec.z = msg->z;
        };
    };
}

/*-----------------------------------------------------------------*/
_dispatcher(void, base_BSM_SCALE, struct flt_vector_msg *msg)
/*
**  FUNCTION
**      Setzt die Raum-Skalierer [scl] des Objects. In
**      der <mask> der vector_msg wird angegeben, welche
**      Raumdimensionen ignoriert werden sollen.
**      Falls VEC_RELATIVE gesetzt ist, bitte beachten,
**      das die relative Modifikation multiplizierend wirkt!
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      28-Nov-94   floh    created
**      03-Dec-94   floh    die TForm hat jetzt eine strukturiertere
**                          Struktur, alles angepaßt...
**      20-Jan-95   floh    Nucleus2-Revision
*/
{
    /* Pointer auf TForm... */
    register tform *tf = &(((struct base_data *)INST_DATA(cl,o))->TForm);

    /* Maske aus vector_msg holen */
    register ULONG mask = msg->mask;

    /* Skalierer absolut oder relativ beeinflussen? */
    if (mask & VEC_RELATIVE) {

        /* welche Scaler RELATIV modifizieren? */
        if (mask & VEC_X)   tf->scl.x *= msg->x;
        if (mask & VEC_Y)   tf->scl.y *= msg->y;
        if (mask & VEC_Z)   tf->scl.z *= msg->z;

    } else {

        /* welche Scaler ABSOLUT modifizieren? */
        if (mask & VEC_X)   tf->scl.x = msg->x;
        if (mask & VEC_Y)   tf->scl.y = msg->y;
        if (mask & VEC_Z)   tf->scl.z = msg->z;
    };

    /* Rotations/Skalierungs-Matrix auffrischen */
    _RefreshTForm(tf);
}


/*-----------------------------------------------------------------*/
_dispatcher(void, base_BSM_ANGLE, struct lng_vector_msg *msg)
/*
**  FUNCTION
**      Setzt die Achswinkel [ang] des Objects. In der
**      <mask> der vector_msg wird angegeben, welche Achswinkel
**      gesetzt bzw. ignoriert werden sollen:
**
**      DoMethod(o,BSM_ANGLE,(VEC_Y|VEC_Z),0,123,90);
**
**      Bitte beachten, daß die numerischen Werte als LONG
**      erwartet werden und zwischen 0 und 359 liegen müssen!
**
**      BITTE UNBEDINGT DEN TEXT ZUR <struct vector_msg> DURCHLESEN!
**      (in "baseclass/baseclass.h")
**
**      Um die in der TForm eingebettete Rotations-Matrix uptodate
**      zu halten, wird ein TE_GET_SPEC->RefreshTForm() darauf
**      angewendet.
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      12-Nov-94   floh    created
**      13-Nov-94   floh    Die Achswinkel in der <tform> werden
**                          jetzt nicht mehr als FLOAT, sondern
**                          als 16.16 Festkomma-LONG gehalten.
**      17-Nov-94   floh    serial debug code
**      28-Nov-94   floh    jetzt mit lng_vector_msg
**      03-Dec-94   floh    die TForm hat jetzt eine strukturiertere
**                          Struktur, alles angepaßt...
**      20-Jan-95   floh    Nucleus2-Revision
*/
{
    /* Pointer auf TForm... */
    register tform *tf = &(((struct base_data *)INST_DATA(cl,o))->TForm);

    /* Maske aus vector_msg holen */
    register ULONG mask = msg->mask;

    /* Winkel absolut oder relativ beeinflussen? */
    if (mask & VEC_RELATIVE) {

        /* welche Winkel RELATIV modifizieren (mit Overflow-Korrektur) */
        if (mask & VEC_X)
            tf->ang.x = (tf->ang.x + (msg->x << 16)) % (360<<16);
        if (mask & VEC_Y)
            tf->ang.y = (tf->ang.y + (msg->y << 16)) % (360<<16);
        if (mask & VEC_Z)
            tf->ang.z = (tf->ang.z + (msg->z << 16)) % (360<<16);
    } else {

        /* welche Winkel ABSOLUT modifizieren? */
        if (mask & VEC_X)   tf->ang.x = msg->x << 16;
        if (mask & VEC_Y)   tf->ang.y = msg->y << 16;
        if (mask & VEC_Z)   tf->ang.z = msg->z << 16;
    };

    /* Rotations/Skalierungs-Matrix auffrischen */
    _RefreshTForm(tf);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, base_BSM_ROTATION, struct lng_vector_msg *msg)
/*
**  FUNCTION
**      Setzt die neue Achsrotation [rot] des Objects. In
**      der <mask> der vector_msg wird angegeben, welche
**      Rotations-Achsen ignoriert bzw. gesetzt werden sollen:
**
**      DoMethod(o,BSM_ROTATION,(VEC_X),12,0,0);
**
**      Die Zahlenwerte geben die Rotation in Grad/Sekunde(!!!)
**      an (und nicht in Ticks!).
**      Bitte beachten, daß die numerischen Werte als LONG
**      erwartet werden und zwischen  0 und 359 liegen
**      müssen!
**      Falls das <mask>-Bit VEC_FORCE_ZERO gesetzt ist, wird die
**      Rotation vollständig ausgeschaltet, es findet dann auch
**      keine aktive Kollisions-Kontrolle mehr statt
**      (via BSF_RotateMe-Flag).
**
**      BITTE UNBEDINGT DEN TEXT ZUR <struct vector_msg> DURCHLESEN!
**      (in "baseclass/baseclass.h")
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      12-Nov-94   floh    created
**      17-Nov-94   floh    serial debug code
**      28-Nov-94   floh    jetzt mit lng_vector_msg
**      03-Dec-94   floh    die TForm hat jetzt eine strukturiertere
**                          Struktur, alles angepaßt...
**                          -> oops, gleich noch einen bösen
**                             Tippfehler behoben...
**      20-Jan-95   floh    Nucleus2-Revision
**
**  NOTE
**      Die (<<6) Verschiebung dürfte für Verwirrung sorgen:
**      Da die Rotation als "Grad pro Sekunde" angegeben wird, muß
**      ich zuerst die Rotation pro Tick (also pro 1/1000 Sekunde)
**      ausrechnen. Der Einfachheit halber mach ich eine Division
**      durch 1024, also (>>10). Vorher muß ich aber den Integer-Value
**      auf das Festkomma-Format (16.16) bringen, also ein (<<16).
**      (<<16) und (>>10) ergibt zusammen also (<<6). Easy.
*/
{
    /* Pointer auf LID */
    register struct base_data *bd = INST_DATA(cl,o);

    /* Maske aus vector_msg holen */
    register ULONG mask = msg->mask;

    /* Sonderfall: Rotation ausschalten? */
    if (mask & VEC_FORCE_ZERO) {

        /* dann BSF_RotateMe-Flag löschen */
        bd->Flags &= ~BSF_RotateMe;

    } else {

        /* Pointer auf TForm */
        register tform *tf = &(bd->TForm);

        /* BSF_RotateMe-Flag setzen */
        bd->Flags |= BSF_RotateMe;

        /* relativ oder absolut modifizieren? */
        if (mask & VEC_RELATIVE) {

            /* welche Winkel-Adder RELATIV modifizieren? (+ Overflow Korr.) */
            if (mask & VEC_X)
                tf->rot.x = (tf->rot.x + (msg->x << 6)) % (360<<16);
            if (mask & VEC_Y)
                tf->rot.y = (tf->rot.y + (msg->y << 6)) % (360<<16);
            if (mask & VEC_Z)
                tf->rot.z = (tf->rot.z + (msg->z << 6)) % (360<<16);

        } else {

            /* welche Koordinaten ABSOLUT modifizieren? */
            if (mask & VEC_X)   tf->rot.x = (msg->x << 6);
            if (mask & VEC_Y)   tf->rot.y = (msg->y << 6);
            if (mask & VEC_Z)   tf->rot.z = (msg->z << 6);
        };
    };
}

