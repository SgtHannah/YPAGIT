#ifndef TRANSFORM_TFORM_H
#define TRANSFORM_TFORM_H
/*
**  $Source: PRG:VFM/Include/transform/tform.h,v $
**  $Revision: 38.7 $
**  $Date: 1995/09/24 18:44:07 $
**  $Locker:  $
**  $Author: floh $
**
**  Typ-Definition der <tform>, die komplette Definition eines
**  Koordinaten-Systems.
**
**  (C) Copyright 1994 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

/*-------------------------------------------------------------------
**  Prototype...
*/
struct tform;
typedef struct tform tform;

/*-------------------------------------------------------------------
**  [mother]    -> Pointer auf TForm des Koordinaten-Systems, in daß
**                 diese TFORM eingebettet ist.
**                 <mother == NULL>, falls die <tform> direkt ins
**                 Global-System eingebettet ist. Über das Flag
**                 TFF_FollowMother kann explizit beeinflußt werden,
**                 ob ein Object seiner Mother folgen soll (das
**                 ist natürlich nur möglich, wenn <tform.mother>
**                 einen gültigen Pointer enthält.
**                 Der Default-Wert für dieses Flag ist FALSE!
**  [loc]       -> Position in Mother-Koordinaten-System.
**                 Ist gleichzeitig auch die (abgetrennte) Translations-
**                 Matrix.
**  [glb]       -> Position im globalen Koordinaten-System.
**                 Wird ermittelt durch <TFormToGlobal()>
**  [vec]       -> Beweguns-Vektor. Hier sind Richtung und Geschwindigkeit
**                 definiert. Der Vektor gibt den zurückgelegten
**                 Weg pro VFM-Tick an (ca.1/1000 sec).
**  [scl]       -> Skalierungs-Faktoren.
**  [loc_m]     -> Lokale Rotations/Skalierungs-Matrix. Diese 3x3-Matrix
**                 wird ermittelt aus den Achswinkeln [ax,ay,az] und
**                 den Skalierern [sx,sy,sz].
**  [glb_m]     -> Globale Rotations/Skalierungs-Matrix.
**                 Wird ermittelt durch <TFormToGlobal()>
**  [ang]       -> Die Achswinkel des Koordinaten-Systems relativ zum
**                 übergeordneten System. 
**                 ACHTUNG: Das Zahlenformat hierfür ist (16.16) Festkomma!
**  [rot]       -> Der "Dreh-Vektor" des Koordinaten-System pro Tick.
**                 Für jeden VFM-Tick werden diese Werte auf [ax,ay,az]
**                 aufaddiert.
**                 ACHTUNG: Das Zahlenformat hierfür ist (16.16) Festkomma!
**  [flags]     -> siehe unten...
*/
struct flt_triple {
    FLOAT x,y,z;
};

struct lng_triple {
    LONG x,y,z;
};

struct flt_m3x3 {
    FLOAT m11,m12,m13;
    FLOAT m21,m22,m23;
    FLOAT m31,m32,m33;
};

struct quat {
    FLOAT x,y,z,w;
};

struct tform {
    tform *mother;

    struct flt_triple loc;      /* lokale Position */
    struct flt_triple glb;      /* globale Position */
    struct flt_triple vec;      /* Bewegungs-Richtung + Geschwindigkeit */
    struct flt_triple scl;      /* Skalierer */

    struct flt_m3x3 loc_m;      /* lokale Rot/Scale-3x3-Matrix */
    struct flt_m3x3 glb_m;      /* globale Rot/Scale-3x3-Matrix */

    struct lng_triple ang;      /* Achwinkel (16.16) */
    struct lng_triple rot;      /* Rotation (° per Tick) (16.16) */

    ULONG flags;
};

/*-------------------------------------------------------------------
**  Definitionen für <tform.flags>
**  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
**  TFF_FollowMother    -> Definiert, daß das Objekt in das Mother-System
**                         eingebettet ist.
**  TFF_GlobalizeNoRot  -> Bei der Objekt->World Transformation nur
**                         translatieren, nicht rotieren, spart 9
**                         Multiplikationen und 6 Additionen pro Punkt.
*/
#define TFF_FollowMother    (1<<0)
#define TFF_GlobalizeNoRot  (1<<1)

/*-----------------------------------------------------------------*/
#endif



