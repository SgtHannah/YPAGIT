/*
**  $Source: PRG:VFM/Engines/TransEngineNG/srzxy_getmatrix.c,v $
**  $Revision: 38.5 $
**  $Date: 1995/02/14 20:42:34 $
**  $Locker:  $
**  $Author: floh $
**
**  srzxy_getmatrix.c
**
**  Das Frontend für dieses Modul ist die Routine GetMatrix(),
**  die aus den Achswinkeln [ax,ay,az] und den Skalierungs-Faktoren
**  [sx,sy,sz] auf dem effizientesten Weg die lokale
**  3x3-Rotations/Skalierungs-Matrix [m11..m33] berechnet.
**  Nähere Informationen: siehe "research/transformation.txt".
**
**  (C) Copyright 1994 by A.Weissflog
*/
/* Amiga-[Emul]-Includes */
#include <exec/types.h>

/* ANSI Includes */
#include <math.h>

/* VFM Includes */
#include "types.h"
#include "transform/te_ng.h"

/*-----------------------------------------------------------------*/
void te_GetMatrixXYZ(tform *);
void te_GetMatrixXY(tform *);
void te_GetMatrixXZ(tform *);
void te_GetMatrixYZ(tform *);
void te_GetMatrixX(tform *);
void te_GetMatrixY(tform *);
void te_GetMatrixZ(tform *);
void te_GetMatrixNorot(tform *);

/*-----------------------------------------------------------------*/
extern struct TE_Base TEBase;

/*-------------------------------------------------------------------
**  Eine lokale Jump-Table aufbauen mit spezialisierten
**  GetMatrix-Routinen...
*/
struct jp_entry {
    void (*get_matrix) (tform *);
};

/* the JumpTable itself */
struct jp_entry jp[] = {
    te_GetMatrixNorot,
    te_GetMatrixX,
    te_GetMatrixY,
    te_GetMatrixXY,
    te_GetMatrixZ,
    te_GetMatrixXZ,
    te_GetMatrixYZ,
    te_GetMatrixXYZ,
};

/*** zum Debuggen vielleicht ganz nützlich: ***/
/*
struct jp_entry jp[] = {
    te_GetMatrixXYZ,
    te_GetMatrixXYZ,
    te_GetMatrixXYZ,
    te_GetMatrixXYZ,
    te_GetMatrixXYZ,
    te_GetMatrixXYZ,
    te_GetMatrixXYZ,
    te_GetMatrixXYZ,
};
*/

/*-----------------------------------------------------------------*/
void te_GetMatrixXYZ(tform *tf)
/*
**  FUNCTION
**      [ang],[scl] -> [loc_m]
**
**      Für die zugehörige Matrix siehe "research/transformation.txt"
**
**  INPUTS
**      tform   -> Pointer auf zu behandelnde TForm.
**
**  RESULTS
**
**  CHANGED
**      24-Nov-94   floh    created
**      03-Dec-94   floh    angepaßt an neues TForm-Format
**      02-Jan-94   floh    Nucleus2-Revision
*/
{
    /* Sinuse und Cosinuse der Achswinkel ermitteln... */
    struct Sincos_atom *sc_table = TEBase.Sincos_Table;

    struct Sincos_atom *ax_ptr = &sc_table[tf->ang.x >> 16];
    FLOAT sin_ax = ax_ptr->sinus;
    FLOAT cos_ax = ax_ptr->cosinus;

    struct Sincos_atom *ay_ptr = &sc_table[tf->ang.y >> 16];
    FLOAT sin_ay = ay_ptr->sinus;
    FLOAT cos_ay = ay_ptr->cosinus;

    struct Sincos_atom *az_ptr = &sc_table[tf->ang.z >> 16];
    FLOAT sin_az = az_ptr->sinus;
    FLOAT cos_az = az_ptr->cosinus;

    FLOAT store;

    /* Matrix ausfüllen */
    store = sin_az*sin_ax;
    tf->loc_m.m11 = tf->scl.x * (cos_az*cos_ay - store*sin_ay);
    tf->loc_m.m12 = tf->scl.x * (-sin_az*cos_ax);
    tf->loc_m.m13 = tf->scl.x * (cos_az*sin_ay + store*cos_ay);

    store = cos_az*sin_ax;
    tf->loc_m.m21 = tf->scl.y * (sin_az*cos_ay + store*sin_ay);
    tf->loc_m.m22 = tf->scl.y * (cos_az*cos_ax);
    tf->loc_m.m23 = tf->scl.y * (sin_az*sin_ay - store*cos_ay);

    tf->loc_m.m31 = tf->scl.z * (-cos_ax*sin_ay);
    tf->loc_m.m32 = tf->scl.z * (sin_ax);
    tf->loc_m.m33 = tf->scl.z * (cos_ax*cos_ay);

    /* Ende */
}

/*-----------------------------------------------------------------*/
void te_GetMatrixXY(tform *tf)
/*
**  FUNCTION
**      [ang],[scl] -> [loc_m]
**
**      Für die zugehörige Matrix siehe "research/transformation.txt"
**
**  INPUTS
**      tform   -> Pointer auf zu behandelnde TForm.
**
**  RESULTS
**
**  CHANGED
**      24-Nov-94   floh    created
**      03-Dec-94   floh    angepaßt an neues TForm-Format
**      02-Jan-94   floh    Nucleus2-Revision
*/
{
    /* Sinuse und Cosinuse der Achswinkel ermitteln... */
    struct Sincos_atom *sc_table = TEBase.Sincos_Table;

    struct Sincos_atom *ax_ptr = &sc_table[tf->ang.x >> 16];
    FLOAT sin_ax = ax_ptr->sinus;
    FLOAT cos_ax = ax_ptr->cosinus;

    struct Sincos_atom *ay_ptr = &sc_table[tf->ang.y >> 16];
    FLOAT sin_ay = ay_ptr->sinus;
    FLOAT cos_ay = ay_ptr->cosinus;

    /* Matrix ausfüllen */
    tf->loc_m.m11 = tf->scl.x * cos_ay;
    tf->loc_m.m12 = 0;
    tf->loc_m.m13 = tf->scl.x * sin_ay;

    tf->loc_m.m21 = tf->scl.y * (sin_ax*sin_ay);
    tf->loc_m.m22 = tf->scl.y * cos_ax;
    tf->loc_m.m23 = tf->scl.y * (-sin_ax*cos_ay);

    tf->loc_m.m31 = tf->scl.z * (-cos_ax*sin_ay);
    tf->loc_m.m32 = tf->scl.z * (sin_ax);
    tf->loc_m.m33 = tf->scl.z * (cos_ax*cos_ay);

    /* Ende */
}

/*-----------------------------------------------------------------*/
void te_GetMatrixXZ(tform *tf)
/*
**  FUNCTION
**      [ang],[scl] -> [loc_m]
**
**      Für die zugehörige Matrix siehe "research/transformation.txt"
**
**  INPUTS
**      tform   -> Pointer auf zu behandelnde TForm.
**
**  RESULTS
**
**  CHANGED
**      24-Nov-94   floh    created
**      03-Dec-94   floh    angepaßt an neues TForm-Format
**      02-Jan-94   floh    Nucleus2-Revision
*/
{
    /* Sinuse und Cosinuse der Achswinkel ermitteln... */
    struct Sincos_atom *sc_table = TEBase.Sincos_Table;

    struct Sincos_atom *ax_ptr = &sc_table[tf->ang.x >> 16];
    FLOAT sin_ax = ax_ptr->sinus;
    FLOAT cos_ax = ax_ptr->cosinus;

    struct Sincos_atom *az_ptr = &sc_table[tf->ang.z >> 16];
    FLOAT sin_az = az_ptr->sinus;
    FLOAT cos_az = az_ptr->cosinus;

    /* Matrix ausfüllen */
    tf->loc_m.m11 = tf->scl.x * cos_az;
    tf->loc_m.m12 = tf->scl.x * (-sin_az*cos_ax);
    tf->loc_m.m13 = tf->scl.x * (sin_az*sin_ax);

    tf->loc_m.m21 = tf->scl.y * sin_az;
    tf->loc_m.m22 = tf->scl.y * (cos_az*cos_ax);
    tf->loc_m.m23 = tf->scl.y * (-cos_az*sin_ax);

    tf->loc_m.m31 = 0;
    tf->loc_m.m32 = tf->scl.z * sin_ax;
    tf->loc_m.m33 = tf->scl.z * cos_ax;

    /* Ende */
}

/*-----------------------------------------------------------------*/
void te_GetMatrixYZ(tform *tf)
/*
**  FUNCTION
**      [ang],[scl] -> [loc_m]
**
**      Für die zugehörige Matrix siehe "research/transformation.txt"
**
**  INPUTS
**      tform   -> Pointer auf zu behandelnde TForm.
**
**  RESULTS
**
**  CHANGED
**      24-Nov-94   floh    created
**      03-Dec-94   floh    angepaßt an neues TForm-Format
**      02-Jan-94   floh    Nucleus2-Revision
*/
{
    /* Sinuse und Cosinuse der Achswinkel ermitteln... */
    struct Sincos_atom *sc_table = TEBase.Sincos_Table;

    struct Sincos_atom *ay_ptr = &sc_table[tf->ang.y >> 16];
    FLOAT sin_ay = ay_ptr->sinus;
    FLOAT cos_ay = ay_ptr->cosinus;

    struct Sincos_atom *az_ptr = &sc_table[tf->ang.z >> 16];
    FLOAT sin_az = az_ptr->sinus;
    FLOAT cos_az = az_ptr->cosinus;

    /* Matrix ausfüllen */
    tf->loc_m.m11 = tf->scl.x * (cos_az*cos_ay);
    tf->loc_m.m12 = tf->scl.x * (-sin_az);
    tf->loc_m.m13 = tf->scl.x * (cos_az*sin_ay);

    tf->loc_m.m21 = tf->scl.y * (sin_az*cos_ay);
    tf->loc_m.m22 = tf->scl.y * cos_az;
    tf->loc_m.m23 = tf->scl.y * (sin_az*sin_ay);

    tf->loc_m.m31 = tf->scl.z * (-sin_ay);
    tf->loc_m.m32 = 0;
    tf->loc_m.m33 = tf->scl.z * (cos_ay);

    /* Ende */
}

/*-----------------------------------------------------------------*/
void te_GetMatrixX(tform *tf)
/*
**  FUNCTION
**      [ang],[scl] -> [loc_m]
**
**      Für die zugehörige Matrix siehe "research/transformation.txt"
**
**  INPUTS
**      tform   -> Pointer auf zu behandelnde TForm.
**
**  RESULTS
**
**  CHANGED
**      24-Nov-94   floh    created
**      03-Dec-94   floh    angepaßt an neues TForm-Format
**      02-Jan-94   floh    Nucleus2-Revision
*/
{
    /* Sinuse und Cosinuse der Achswinkel ermitteln... */
    struct Sincos_atom *sc_table = TEBase.Sincos_Table;

    struct Sincos_atom *ax_ptr = &sc_table[tf->ang.x >> 16];
    FLOAT sin_ax = ax_ptr->sinus;
    FLOAT cos_ax = ax_ptr->cosinus;

    /* Matrix ausfüllen */
    tf->loc_m.m11 = tf->scl.x;
    tf->loc_m.m12 = 0;
    tf->loc_m.m13 = 0;

    tf->loc_m.m21 = 0;
    tf->loc_m.m22 = tf->scl.y * cos_ax;
    tf->loc_m.m23 = tf->scl.y * (-sin_ax);

    tf->loc_m.m31 = 0;
    tf->loc_m.m32 = tf->scl.z * sin_ax;
    tf->loc_m.m33 = tf->scl.z * cos_ax;

    /* Ende */
}

/*-----------------------------------------------------------------*/
void te_GetMatrixY(tform *tf)
/*
**  FUNCTION
**      [ang],[scl] -> [loc_m]
**
**      Für die zugehörige Matrix siehe "research/transformation.txt"
**
**  INPUTS
**      tform   -> Pointer auf zu behandelnde TForm.
**
**  RESULTS
**
**  CHANGED
**      24-Nov-94   floh    created
**      03-Dec-94   floh    angepaßt an neues TForm-Format
**      02-Jan-94   floh    Nucleus2-Revision
*/
{
    /* Sinuse und Cosinuse der Achswinkel ermitteln... */
    struct Sincos_atom *sc_table = TEBase.Sincos_Table;

    struct Sincos_atom *ay_ptr = &sc_table[tf->ang.y >> 16];
    FLOAT sin_ay = ay_ptr->sinus;
    FLOAT cos_ay = ay_ptr->cosinus;

    /* Matrix ausfüllen */
    tf->loc_m.m11 = tf->scl.x * cos_ay;
    tf->loc_m.m12 = 0;
    tf->loc_m.m13 = tf->scl.x * sin_ay;

    tf->loc_m.m21 = 0;
    tf->loc_m.m22 = tf->scl.y;
    tf->loc_m.m23 = 0;

    tf->loc_m.m31 = tf->scl.z * (-sin_ay);
    tf->loc_m.m32 = 0;
    tf->loc_m.m33 = tf->scl.z * cos_ay;

    /* Ende */
}

/*-----------------------------------------------------------------*/
void te_GetMatrixZ(tform *tf)
/*
**  FUNCTION
**      [ang],[scl] -> [loc_m]
**
**      Für die zugehörige Matrix siehe "research/transformation.txt"
**
**  INPUTS
**      tform   -> Pointer auf zu behandelnde TForm.
**
**  RESULTS
**
**  CHANGED
**      24-Nov-94   floh    created
**      03-Dec-94   floh    angepaßt an neues TForm-Format
**      02-Jan-94   floh    Nucleus2-Revision
*/
{
    /* Sinuse und Cosinuse der Achswinkel ermitteln... */
    struct Sincos_atom *sc_table = TEBase.Sincos_Table;

    struct Sincos_atom *az_ptr = &sc_table[tf->ang.z >> 16];
    FLOAT sin_az = az_ptr->sinus;
    FLOAT cos_az = az_ptr->cosinus;

    /* Matrix ausfüllen */
    tf->loc_m.m11 = tf->scl.x * cos_az;
    tf->loc_m.m12 = tf->scl.x * (-sin_az);
    tf->loc_m.m13 = 0;

    tf->loc_m.m21 = tf->scl.y * sin_az;
    tf->loc_m.m22 = tf->scl.y * cos_az;
    tf->loc_m.m23 = 0;

    tf->loc_m.m31 = 0;
    tf->loc_m.m32 = 0;
    tf->loc_m.m33 = tf->scl.z;

    /* Ende */
}

/*-----------------------------------------------------------------*/
void te_GetMatrixNorot(tform *tf)
/*
**  FUNCTION
**      [ang],[scl] -> [loc_m]
**
**      Für die zugehörige Matrix siehe "research/transformation.txt"
**
**  INPUTS
**      tform   -> Pointer auf zu behandelnde TForm.
**
**  RESULTS
**
**  CHANGED
**      24-Nov-94   floh    created
**      03-Dec-94   floh    angepaßt an neues TForm-Format
**      02-Jan-94   floh    Nucleus2-Revision
*/
{
    /* Matrix ausfüllen */
    tf->loc_m.m11 = tf->scl.x;
    tf->loc_m.m12 = 0;
    tf->loc_m.m13 = 0;

    tf->loc_m.m21 = 0;
    tf->loc_m.m22 = tf->scl.y;
    tf->loc_m.m23 = 0;

    tf->loc_m.m31 = 0;
    tf->loc_m.m32 = 0;
    tf->loc_m.m33 = tf->scl.z;

    /* Ende */
}

/*-----------------------------------------------------------------*/
void te_GetMatrix(tform *tf)
/*
**  FUNCTION
**      Ermittelt auf optimale Weise aus den Achswinkeln
**      der <TForm> die zugehörige Rotations-Matrix und
**      schreibt diese in die entsprechenden Felder der TForm.
**
**  INPUTS
**      tf->ang.x      Winkel um X-Achse
**      tf->ang.y      Winkel um Y-Achse
**      tf->ang.z      Winkel um Z-Achse
**      tf->scl.x      X-Skalierer
**      tf->scl.y      Y-Skalierer
**      tf->scl.z      Z-Skalierer
**
**  RESULTS
**      [tf->loc_m]
**
**  CHANGED
**      24-Nov-94   floh    created
**      03-Dec-94   floh    angepaßt an neues TForm-Format
**      06-Dec-94   floh    Dummerweise wurden alle Winkel beim
**                          Testen auf UWORD gecastet, das führte
**                          zu ziemlich eigenartigen Resultaten, weil
**                          die Winkel ja als (16.16) vorliegen!
**      02-Jan-94   floh    Nucleus2-Revision
*/
{
    /* ermittle optimale GetMatrix()-Routine über Jump-Table */
    WORD jump_entry_code = 0;
    if ((tf->ang.x) != 0)  jump_entry_code += 1;
    if ((tf->ang.y) != 0)  jump_entry_code += 2;
    if ((tf->ang.z) != 0)  jump_entry_code += 4;

    jp[jump_entry_code].get_matrix(tf);
}

