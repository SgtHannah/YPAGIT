/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_intersect.c,v $
**  $Revision: 38.20 $
**  $Date: 1998/01/06 16:21:48 $
**  $Locker: floh $
**  $Author: floh $
**
**  Support-Routinen und Methoden-Dispatcher für Kollisions-
**  Kontrolle gegen Welt.
**
**  (C) Copyright 1995 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/lists.h>
#include <exec/nodes.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "ypa/ypaworldclass.h"

#include "yw_protos.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus

struct yw_CollisionZone {
    struct Skeleton *sklt;
    FLOAT mid_x,mid_y,mid_z;
    ULONG secx,secy;
    struct Cell *sector;
    UWORD type;
    UWORD flags;            // INTERSECTF_CHEAT...
};

#define CZT_INVALID        (0)
#define CZT_LEGO           (1)
#define CZT_VSIDE_SLURP    (2)
#define CZT_HSIDE_SLURP    (3)
#define CZT_CROSS_SLURP    (4)

/*-----------------------------------------------------------------*/
BOOL yw_PiP(FLOAT x, FLOAT y, FLOAT z,
            UWORD pnum,
            struct Skeleton *sklt)
/*
**  FUNCTION
**      3D-Point-In-Polygon-Checker. Testet, ob p[x,y,z] innerhalb
**      des mittels <pnum> und <sklt> definierten 3D-Polygons
**      liegt. Es muß bereits feststehen, daß der Punkt in der
**      Ebene des Polygons liegt.
**
**      Der Algorithmus arbeitet in 2D, dabei werden die
**      beiden 3D-Komponenten ausgewählt, die das genaueste
**      Ergebnis liefern.
**
**  INPUTS
**      x,y,z   -> 3D-Punkt relativ zu <sklt>
**      pnum    -> Polygon-Nummer in Skeleton
**      sklt    -> enthält den zu testenden Polygon, der
**                 PlanePool muß gültige Ebenen-Parameter enthalten!
**
**  RESULTS
**      TRUE    -> Point ist innerhalb Polygon
**      FALSE   -> Point ist nicht im Polygon
**
**  CHANGED
**      03-Jul-95   floh    created
**      05-Jul-95   floh    debugging...
**      17-Jul-95   floh    BugFix: Bei der Entscheidung, welche 3D-Komponente
**                          fallengelassen wird, wurden die Flächen-
**                          normalen-Komponenten direkt verglichen,
**                          es muß aber der größte abs() Wert
**                          fallengelassen werden!
**      20-Jan-96   floh    angepaßt an neues Skeleton-Struktur-Polygon-
**                          format
*/
{
    WORD *poly  = sklt->Areas[pnum];
    BOOL insect = FALSE;
    ULONG nump  = *poly++;  // Anzahl Eckpunkte

    /*** größte Normalen-Komponente bestimmt Drop-Komponente... ***/
    FLOAT nx = sklt->PlanePool[pnum].A;
    FLOAT ny = sklt->PlanePool[pnum].B;
    FLOAT nz = sklt->PlanePool[pnum].C;
    FLOAT drop;

    nx = (nx < 0) ? -nx:nx;
    ny = (ny < 0) ? -ny:ny;
    nz = (nz < 0) ? -nz:nz;

    drop = (nx > ny) ? ((nx > nz) ? nx:nz) : ((ny>nz) ? ny:nz);

    if (drop == nx) {

        /*** drop x ***/
        ULONG i,j;
        for (i=0, j=nump-1; i<nump; j=i++) {

            FLOAT y0 = sklt->Pool[poly[i]].y;
            FLOAT z0 = sklt->Pool[poly[i]].z;
            FLOAT y1 = sklt->Pool[poly[j]].y;
            FLOAT z1 = sklt->Pool[poly[j]].z;

            if ((((z1 <= z) && (z<z0)) ||
                 ((z0 <= z) && (z<z1))) &&
                (y < (y0-y1) * (z-z1)/(z0-z1) + y1))
            {
                insect = !insect;
            };
        };

    } else if (drop == ny) {

        /*** drop y ***/
        ULONG i,j;
        for (i=0, j=nump-1; i<nump; j=i++) {

            FLOAT x0 = sklt->Pool[poly[i]].x;
            FLOAT z0 = sklt->Pool[poly[i]].z;
            FLOAT x1 = sklt->Pool[poly[j]].x;
            FLOAT z1 = sklt->Pool[poly[j]].z;

            if ((((z1 <= z) && (z<z0)) ||
                 ((z0 <= z) && (z<z1))) &&
                (x < (x0-x1) * (z-z1)/(z0-z1) + x1))
            {
                insect = !insect;
            };
        };

    } else if (drop == nz) {

        /*** drop z ***/
        ULONG i,j;
        for (i=0, j=nump-1; i<nump; j=i++) {

            FLOAT x0 = sklt->Pool[poly[i]].x;
            FLOAT y0 = sklt->Pool[poly[i]].y;
            FLOAT x1 = sklt->Pool[poly[j]].x;
            FLOAT y1 = sklt->Pool[poly[j]].y;

            if ((((y1 <= y) && (y<y0)) ||
                 ((y0 <= y) && (y<y1))) &&
                (x < (x0-x1) * (y-y1)/(y0-y1) + x1))
            {
                insect = !insect;
            };
        };
    };

    /*** Ende ***/
    return(insect);
}

/*-----------------------------------------------------------------*/
BOOL yw_CircleGetBestPoint(struct Skeleton *sklt,
                           ULONG pnum,
                           FLOAT x, FLOAT y, FLOAT z,
                           FLOAT radius,
                           fp3d *result)
/*
**  FUNCTION
**      Was ist der beste Punkt für einen Point-In-Polygon-Check,
**      wenn [x,y,z] auf Ebene des Polygons liegt und Mittelpunkt
**      eines Kreises ist, der durch Radius <radius> mitdefiniert
**      wird?
**
**      Antwort: Der Punkt, der auf dem Vektor von [x,y,z] zum
**      Schwerpunkt der Fläche liegt, und genau [radius] von
**      [x,y,z] entfernt ist.
**
**      Dieser Punkt wird berechnet und in <result> eingetragen.
**      Falls der Schwerpunkt der Fläche innerhalb des Test-Radius
**      liegt, erübrigt sich ein Point-In-Polygon-Check, in diesem
**      Fall wird FALSE zurückgegeben, sonst TRUE.
**
**  INPUTS
**      sklt    -> Pointer auf Skeleton-Struktur, die den gewünschten
**                 Polygon enthält
**      pnum    -> Nummer des Polygons in <sklt>
**      x,y,z   -> Point auf Ebene des Polygons, Mittelpunkt des 
**                 Test-Kreises
**      radius  -> Radius des Test-Kreises
**      result  -> in [x,y,z] wird Best-Fit-Point zurückgegeben, der
**                 als Eingangs-Parameter für yw_PiP() verwendet werden
**                 sollte.
**
**  RESULTS
**      FALSE   -> PiP() ist nicht nötig, der Testkreis intersected
**                 den Polygon in jedem Fall.
**      TRUE    -> in <result> wird ein 3D-Punkt zurückgegeben, der
**                 als Test-Punkt für einen nachfolgenden PiP-Check
**                 verwendet werden sollte.
**
**  CHANGED
**      09-Jul-95   floh    created
**      20-Jan-96   floh    angepaßt an neues Skeleton-Poly-Format
**      07-Sep-96   floh    sqrt() ersetzt durch nc_sqrt()
*/
{
    WORD *poly;
    ULONG nump;
    ULONG i;
    FLOAT mid_x,mid_y,mid_z;
    FLOAT vx,vy,vz,vlen;

    /*** Schwerpunkt des Polygons ermitteln ***/
    poly = sklt->Areas[pnum];
    nump = *poly++; // Anzahl Punkte im Poly
    mid_x = mid_y = mid_z = 0.0;

    for (i=0; i<nump; i++) {
        mid_x += sklt->Pool[poly[i]].x;
        mid_y += sklt->Pool[poly[i]].y;
        mid_z += sklt->Pool[poly[i]].z;
    };
    mid_x /= nump;
    mid_y /= nump;
    mid_z /= nump;

    /*** 3D-Vektor Midpoint zu Test-Point ***/
    vx = mid_x - x;
    vy = mid_y - y;
    vz = mid_z - z;
    vlen = nc_sqrt(vx*vx + vy*vy + vz*vz);
    if (vlen > radius) {

        /*** Best-Point ***/
        result->x = ((vx/vlen) * radius) + x;
        result->y = ((vy/vlen) * radius) + y;
        result->z = ((vz/vlen) * radius) + z;
        return(TRUE);

    } else return(FALSE);
}

/*-----------------------------------------------------------------*/
void yw_Intersector(struct intersect_msg *msg,
                    struct yw_CollisionZone *zone)
/*
**  FUNCTION
**      Testet, ob der in <msg> definierte Ortsvektor
**      einen der Polygone in <sklt> schneidet. Dieser
**      Test wird für *jeden* Polygon im Skeleton
**      gemacht, zurückgegeben wird die dem Startpunkt
**      am nächsten liegende Intersection.
**
**      *** WICHTIG ***
**      Der PlanePool des Skeletons muß gültig sein!
**
**      Bonus-Feature:
**      ~~~~~~~~~~~~~~
**      Die Parameter in der <msg> werden nicht extra initialisiert,
**      (insbesondere <t>), damit ist es möglich, mehrere
**      Skeletons hintereinander auf Intersections zu testen.
**      Allerdings muß vor dem allerersten Aufruf von yw_Intersect()
**      der t-Value auf eine Zahl > 1.0 initialisiert werden, damit
**      der "Kleinstes <t> Check" überhaupt anspricht.
**
**  INPUTS
**      msg.pnt     -> definiert 3D-Startpunkt des zu testenden Vektors 
**                     relativ zu <sklt>
**      msg.vec     -> definiert Richtung und Länge des Vektors
**      msg.t       -> auf >1.0 initialisiert oder auf Ergebnis des
**                     vorhergehenden Tests
**      msg.insect  -> auf FALSE initialisiert oder auf Ergebnis des
**                     vorhergehenden Tests
**
**      zone        -> die Collision-Zone-Description (u.a. das Skeleton)
**
**  RESULTS
**      msg.insect  -> TRUE: Intersection aufgetreten, sonst unverändert
**      msg.t       -> t-Value der Intersection
**      msg.ipnt    -> genauer 3D-Intersection-Point
**      msg.pnum    -> Nummer des Polygons in <sklt>, mit dem Intersection
**                     auftrat
**      msg.sklt    -> Pointer auf Skeleton, mit dem Intersection auftrat
**
**  CHANGED
**      03-Jul-95   floh    created
**      05-Jul-95   floh    debugging...
*/
{
    ULONG pnum;

    for (pnum=0; pnum < zone->sklt->NumAreas; pnum++) {

        FLOAT t,dot;

        FLOAT vx = msg->vec.x;
        FLOAT vy = msg->vec.y;
        FLOAT vz = msg->vec.z;

        FLOAT px = msg->pnt.x;
        FLOAT py = msg->pnt.y;
        FLOAT pz = msg->pnt.z;

        struct Plane *pln = &(zone->sklt->PlanePool[pnum]);

        /*** auf Intersection mit aktueller *Ebene* testen ***/

        /* das Dot-Produkt zum Test, ob Ebenen-Intersection mit dem */
        /* "Flächen-Rücken" auftritt" */

        dot = pln->A*vx + pln->B*vy + pln->C*vz;
        if (dot > 0.0) {

            /*** bis hierher OK, jetzt erst <t> ermitteln ***/
            t = -(pln->A*px + pln->B*py + pln->C*pz + pln->D)/dot;

            /*** t innerhalb der Limits && kleiner als aktuelles min_t? ***/
            if ((t > 0.0) && (t <= 1.0) && (t < msg->t)) {

                /*** absolute Position der Intersection... ***/
                FLOAT ix = px + vx*t;
                FLOAT iy = py + vy*t;
                FLOAT iz = pz + vz*t;

                /*** Point-In-Polygon-Check ***/
                if (yw_PiP(ix,iy,iz,pnum,zone->sklt)) {

                    /*** yep, neue gültige Intersection! ***/
                    msg->insect = TRUE;
                    msg->t      = t;
                    msg->ipnt.x = ix + zone->mid_x;
                    msg->ipnt.y = iy + zone->mid_y;
                    msg->ipnt.z = iz + zone->mid_z;
                    msg->pnum   = pnum;
                    msg->sklt   = zone->sklt;
                };
            };
        };
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yw_SphereIntersector(struct insphere_msg *msg,
                          struct yw_CollisionZone *zone)
/*
**  FUNCTION
**      Kugel-Intersector...
**
**      Testet jede Fläche in der CollisionZone auf Intersection
**      mit der in der Message durch 3D-Punkt und Radius definierte
**      Kugel. Alle Intersections werden fortlaufend ins
**      Intersection-Array in der Message geschrieben.
**
**  INPUTS
**      msg.pnt     -> der 3D-Punkt
**      msg.radius  -> der Test-Radius
**      msg.chain   -> hierhin werden die Intersections als
**                     <struct Insect> geschrieben
**      msg.max_insects -> soviele Intersections passen nach <msg.chain>
**      msg.num_insects -> steht auf 1.freien Platz in <msg.chain>
**
**  RESULTS
**      msg.chain[x]    -> ausgefüllt
**      msg.num_insects -> steht auf nächsten freien Platz in <msg.chain>
**
**  CHANGED
**      06-Jul-95   floh    created
**      09-Jul-95   floh    arbeitet jetzt mit yw_CircleGetBestPoint(),
**                          dürfte "etwas" genauer sein...
*/
{
    ULONG pnum;

    FLOAT px = msg->pnt.x;
    FLOAT py = msg->pnt.y;
    FLOAT pz = msg->pnt.z;

    for (pnum=0; pnum < zone->sklt->NumAreas; pnum++) {

        FLOAT t, dot;

        struct Plane *pln = &(zone->sklt->PlanePool[pnum]);
        FLOAT a = pln->A;
        FLOAT b = pln->B;
        FLOAT c = pln->C;
        FLOAT d = pln->D;

        /*** Flächen-Rücken-Unterdrückung ***/
        dot = a*msg->dof.x + b*msg->dof.y + c*msg->dof.z;
        if (dot > 0.0) {

            t = -(a*px + b*py + c*pz + d) / ((a*a + b*b + c*c)*msg->radius);
            if ((t > 0.0) && (t <= 1.0)) {

                BOOL insected = FALSE;
                fp3d bp;

                /*** I-Punkt mit Ebene ermitteln... ***/
                FLOAT len = msg->radius * t;
                FLOAT ix = px + pln->A*len;
                FLOAT iy = py + pln->B*len;
                FLOAT iz = pz + pln->C*len;

                /*** Best-Point ermitteln... ***/
                if (yw_CircleGetBestPoint(zone->sklt, pnum,
                                          ix, iy, iz, msg->radius,
                                          &bp))
                {
                    /*** jetzt noch ein PiP-Check ***/
                    if (yw_PiP(bp.x,bp.y,bp.z,pnum,zone->sklt)) insected = TRUE;
                } else {

                    /*** yw_CircleGetBestPoint() hat bereits festgestellt ***/
                    /*** das eine Intersection aufgetreten ist!           ***/
                    insected = TRUE;
                };

                /*** Intersection???? ***/
                if (insected) {

                    /*** neue Intersection! ***/
                    ULONG index = msg->num_insects;
                    if (index < msg->max_insects) {

                        struct Insect *insect = &(msg->chain[index]);

                        insect->ipnt.x = ix + zone->mid_x;
                        insect->ipnt.y = iy + zone->mid_y;
                        insect->ipnt.z = iz + zone->mid_z;
                        insect->pln   = *pln;
                        msg->num_insects++;
                    };
                };
            };
        };
    };
}

/*-----------------------------------------------------------------*/
void yw_GetSklt(struct ypaworld_data *ywd,
                LONG start_sub_x, LONG start_sub_y,
                LONG sub_x, LONG sub_y,
                struct yw_CollisionZone *res,
                ULONG flags)
/*
**  FUNCTION
**      Ermittelt Kollisions-Skeleton, das für die "Sub-Koordinaten"
**      <sub_x> und <sub_y> zuständig sind. Das kann entweder
**      ein Lego-Skeleton oder ein Slurp-Kollisions-Skeleton
**      sein.
**
**      Falls es sich um Slurp-Skeletons handelt, werden zwar
**      alle Parameter in der <result>-Struktur zurückgegeben,
**      die Slurp-Skeletons werden aber noch *nicht* an die
**      geometrische Form der sie umgebenden Sektoren angepaßt!
**
**  INPUTS
**      ywd         -> Ptr auf LID des Welt-Objects
**      sub_x,sub_y -> Sub-Koordinaten für die die Parameter
**                     ermittelt werden sollen.
**
**
**      result  -> wird von Routine wie folgt ausgefüllt:
**
**          res.sklt = Ptr auf Skeleton, oder NULL, falls außerhalb
**                     der Welt. Ansonsten handelt es sich um ein
**                     Lego-(Subsektor-)-Skeleton, oder um ein noch
**                     nicht angepaßtes Slurp-Kollisions-Skeleton
**          res.mid_x
**          res.mid_y
**          res.mid_z = Punkt, auf dem res.sklt zentriert ist. Zur
**                      Kollisions-Kontrolle muß der Test-Vektor
**                      zu diesem Punkt relativiert werden.
**
**          res.sec_x
**          res.sec_y  = Sektor-Koordinaten des "übergeordneten"
**                       Super-Sektors (notwendig für Slurp-Anpassung)
**
**          res.sector = Ptr auf diesen Sektor.
**          res.type   = eins von
**                           CZT_INVALID         (0)
**                           CZT_LEGO            (1)
**                           CZT_NORTHERN_SLURP  (2)
**                           CZT_SOUTHERN_SLURP  (3)
**                           CZT_WESTERN_SLURP   (4)
**                           CZT_EASTERN_SLURP   (5)
**
**      flags   -> INTERSECTF_CHEAT oder 0
**
**  RESULTS
**      siehe INPUT :-)
**
**  CHANGED
**      03-Jul-95   floh    created
**      05-Jul-95   floh    debugging...
**      24-Jul-95   floh    Das richtige Lego wird jetzt abhängig vom
**                          Energie-Gehalt des Subsektors ermittelt.
**      07-Sep-95   floh    Test auf gültige Subsektor-Pos falsch!
**      20-May-96   floh    Ooops, longstanding *BUG*: der Test auf
**                          Subsektor innerhalb der Welt war broken...
**      12-Aug-96   floh    Macht jetzt am Ende nochmal einen
**                          Test auf nicht invalid und Sklt-Pointer
**                          NULL, in dem Fall wird das INVALID Flag
**                          wieder gesetzt und eine Warnung ausgegeben.
**      24-Feb-98   floh    + INTERSECTF_CHEAT Handling
**                          + Flags wird overridden, wenn start_sub_x
**                            und start_sub_y auf demselben Sektor liegen,
**                            wie sub_x und sub_y
*/
{
    res->type  = CZT_INVALID;
    res->sklt  = NULL;
    res->flags = 0;

    /*** übergebene Subsektor-Position gültig? ***/
    if ((sub_x >= 1) && (sub_x < ((ywd->MapSizeX<<2)-1)) &&
        (sub_y >= 1) && (sub_y < ((ywd->MapSizeY<<2)-1)))
    {
        LONG in_x,in_y;

        /*** wahre Sektor-Position und Sektor-Ptr nach <result> ***/
        res->secx   = sub_x >> 2;
        res->secy   = sub_y >> 2;
        res->sector = &(ywd->CellArea[res->secy * ywd->MapSizeX + res->secx]);

        /*** In-Sektor-Position (0 == Slurp-Streifen) ***/
        in_x = sub_x & 3;
        in_y = sub_y & 3;

        /*** liegt Pos im Slurp-Bereich ? ***/
        if ((in_x == 0) || (in_y == 0)) {

            /*** Position des Slurp-Fragments... ***/
            res->mid_x = sub_x * SLURP_WIDTH;
            res->mid_y = 0.0; /*** gewollt!!! ***/
            res->mid_z = -(sub_y * SLURP_WIDTH);

            /*** Cheat abschalten? ***/
            if ((sub_x == start_sub_x) && (sub_y == start_sub_y))
            {
                flags &= ~INTERSECTF_CHEAT;
            };
            res->flags = flags;

            /*** ...und ein passendes Slurp-Fragment auswählen ***/
            if (in_x == 0) {
                if (in_y == 0) {
                    res->sklt = ywd->CrossSlurp.sklt;
                    res->type = CZT_CROSS_SLURP;
                } else {
                    res->sklt = ywd->SideSlurp.sklt;
                    res->type = CZT_VSIDE_SLURP;
                };
            } else if (in_y == 0) {
                res->sklt = ywd->SideSlurp.sklt;
                res->type = CZT_HSIDE_SLURP;
            };

            /*** das war's ***/

        } else {

            /*** entweder ein Kompakt-Sektor oder ein 3x3-Sektor ***/
            LONG lego_num,sx,sy;
            struct Cell *sec = res->sector;

            res->type = CZT_LEGO;
            if (sec->SType == SECTYPE_COMPACT) {

                /*** ein Kompakt-Sektor ***/
                sx = 0;
                sy = 0;

                /*** Kollissions-Cheat ausschalten, wenn Test auf ***/
                /*** selben Sektor startet, wie der Ziel-Punkt    ***/
                if (((sub_x>>2) == (start_sub_x>>2)) &&
                    ((sub_y>>2) == (start_sub_y>>2)))
                {
                    flags &= ~INTERSECTF_CHEAT;
                };

                /*** Mittelpunkt berechnen ***/
                res->mid_x = res->secx * SECTOR_SIZE + SECTOR_SIZE/2;
                res->mid_y = sec->Height;
                res->mid_z = -(res->secy * SECTOR_SIZE + SECTOR_SIZE/2);

            } else {

                /*** ein 3x3-Subsektor ***/
                sx = in_x - 1;
                sy = 2 - (in_y - 1);

                /*** Kollissions-Cheat ausschalten, wenn Test auf ***/
                /*** selben Sektor startet, wie der Ziel-Punkt    ***/
                if ((sub_x == start_sub_x) && (sub_y == start_sub_y))
                {
                    flags &= ~INTERSECTF_CHEAT;
                };

                /*** Mittelpunkt berechnen ***/
                res->mid_x = sub_x * SLURP_WIDTH;
                res->mid_y = sec->Height;
                res->mid_z = -(sub_y * SLURP_WIDTH);
            };

            /*** Kollisions-Skeleton ermitteln ***/
            res->flags = flags;
            lego_num   = GET_LEGONUM(ywd,sec,sx,sy);
            if (flags & INTERSECTF_CHEAT) {
                res->sklt = ywd->Legos[lego_num].altsklt;
            }else{
                res->sklt = ywd->Legos[lego_num].colsklt;
            };
        };

        /*** finaler Test ***/
        if ((res->type != CZT_INVALID) && (NULL == res->sklt)) {
            UBYTE *t;
            _LogMsg("yw_GetSklt: WARNING, not CZT_INVALID, but Sklt NULL!\n");
            switch (res->type) {
                case CZT_CROSS_SLURP: t="czt_cross_slurp"; break;
                case CZT_VSIDE_SLURP: t="czt_vside_slurp"; break;
                case CZT_HSIDE_SLURP: t="czt_hside_slurp"; break;
                case CZT_LEGO:        t="czt_lego"; break;
            };
            _LogMsg("    Type=%s, sec_x=%d, sec_y=%d.\n",t,res->secx,res->secy);
            res->type = CZT_INVALID;
        };
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yw_GetABCD(struct Skeleton *sklt, ULONG pnum)
/*
**  FUNCTION
**      Berechnet die Ebenen-Parameter des angegebenen Polygons
**      im Skeleton (und überschreibt damit evtl. die
**      vorherigen Ebenen-Parameter dieses Polygons!).
**
**      ABC ist *NICHT* normalisiert (hope it works...).
**
**  INPUTS
**      sklt    -> Ptr auf Skeleton
**      pnum    -> Polygon-Nummer
**
**  RESULTS
**      sklt->PlanePool[pnum] ist auf neuestem Stand.
**
**  CHANGED
**      04-Jul-95   floh    created
**      20-Jan-96   floh    angepaßt an neues Sklt-Poly-Format
**      07-Sep-96   floh    sqrt() ersetzt durch nc_sqrt()
*/
{
    WORD *poly        = sklt->Areas[pnum];
    struct Plane *pln = &(sklt->PlanePool[pnum]);

    /*** Bitte beachten, daß poly[0] Anzahl Punkte enthält! ***/
    fp3d *p0 = &(sklt->Pool[poly[1]]);
    fp3d *p1 = &(sklt->Pool[poly[2]]);
    fp3d *p2 = &(sklt->Pool[poly[3]]);

    /*** zuerst Normalen-Vektor (== A,B,C) ***/
    FLOAT ax = p1->x - p0->x;
    FLOAT ay = p1->y - p0->y;
    FLOAT az = p1->z - p0->z;

    FLOAT bx = p2->x - p1->x;
    FLOAT by = p2->y - p1->y;
    FLOAT bz = p2->z - p1->z;

    FLOAT len;

    pln->A = ay * bz - az * by;
    pln->B = az * bx - ax * bz;
    pln->C = ax * by - ay * bx;
    len = nc_sqrt(pln->A*pln->A + pln->B*pln->B + pln->C*pln->C);

    pln->A /= len;
    pln->B /= len;
    pln->C /= len;

    pln->D = -(pln->A * p0->x  +  pln->B * p0->y  +  pln->C * p0->z);

    /*** fertig ***/
}

/*-----------------------------------------------------------------*/
void yw_PrepareSlurpSklt(struct ypaworld_data *ywd,
                         struct yw_CollisionZone *zone)
/*
**  FUNCTION
**      Falls yw_GetSklt() ein Slurp-Skeleton zurückgegeben
**      hat, muß dieses noch an die Geometrie der Welt
**      angepaßt werden (also Punkte in Richtung Y verschoben
**      und die Ebenen-Parameter neu berechnet werden).
**
**      Genau das erledigt yw_PrepareSlurpSklt().
**
**      Die Routine darf nur aufgerufen werden, wenn
**      ((zone->type != CZT_INVALID) && (zone->type != CZT_LEGO)).
**
**  INPUTS
**      zone    -> das Result aus yw_GetSklt()
**
**  RESULTS
**      Das in <zone> angegebene Slurp-Kollisions-Sklt ist
**      angepaßt.
**
**  CHANGED
**      04-Jul-95   floh    created
**      25-Feb-98   floh    + Support für Kollissions-Cheat bei Slurps
*/
{
    fp3d *pool = zone->sklt->Pool;
    FLOAT side_diff  = 500.0;   // max. Höhendiff für normale Slurps
    FLOAT cross_diff = 300.0;   // max. Höhendiff für Cross-Slurps

    switch(zone->type) {

        case CZT_VSIDE_SLURP:
            {
                /*** für Höhen-Bestimmung... ***/
                struct Cell *this_sec = zone->sector;
                struct Cell *west_sec = this_sec - 1;

                if ((zone->flags & INTERSECTF_CHEAT) &&
                    (abs(this_sec->Height-west_sec->Height)>=side_diff))
                {
                    /*** Kollissions-Cheat anwenden ***/
                    zone->sklt = ywd->AltCollSubSklt;
                    if (this_sec->Height > west_sec->Height){
                        zone->mid_y = this_sec->Height;
                    }else{
                        zone->mid_y = west_sec->Height;
                    };
                } else {
                    /*** Höhenanpassung... ***/
                    pool++->y = west_sec->Height;
                    pool++->y = this_sec->Height;
                    pool++->y = this_sec->Height;
                    pool->y   = west_sec->Height;

                    /*** und Ebenenparameter berechnen... ***/
                    yw_GetABCD(zone->sklt, 0);
                };

            };
            break;

        case CZT_HSIDE_SLURP:
            {
                /*** für Höhen-Bestimmung... ***/
                struct Cell *this_sec  = zone->sector;
                struct Cell *north_sec = this_sec - ywd->MapSizeX;

                if ((zone->flags & INTERSECTF_CHEAT) &&
                    (abs(this_sec->Height-north_sec->Height)>=side_diff))
                {
                    /*** Kollissions-Cheat anwenden ***/
                    zone->sklt = ywd->AltCollSubSklt;
                    if (this_sec->Height > north_sec->Height){
                        zone->mid_y = this_sec->Height;
                    }else{
                        zone->mid_y = north_sec->Height;
                    };
                } else {
                    /*** Höhenanpassung... ***/
                    pool++->y = north_sec->Height;
                    pool++->y = north_sec->Height;
                    pool++->y = this_sec->Height;
                    pool->y   = this_sec->Height;

                    /*** und Ebenenparameter berechnen... ***/
                    yw_GetABCD(zone->sklt, 0);
                };
            };
            break;

        case CZT_CROSS_SLURP:
            {
                /*** für Höhen-Bestimmung... ***/
                BOOL cheated = FALSE;
                struct Cell *this_sec  = zone->sector;
                struct Cell *west_sec  = this_sec - 1;
                struct Cell *north_sec = this_sec - ywd->MapSizeX;
                struct Cell *nw_sec    = north_sec - 1;

                if (zone->flags & INTERSECTF_CHEAT) {
                    /*** mimimax-Höhen ermitteln ***/
                    FLOAT h1 = this_sec->Height;
                    FLOAT h2 = west_sec->Height;
                    FLOAT h3 = north_sec->Height;
                    FLOAT h4 = nw_sec->Height;
                    FLOAT min1 = (h1<h2) ? h1:h2;
                    FLOAT min2 = (h3<h4) ? h3:h4;
                    FLOAT minh = (min1<min2) ? min1:min2;
                    FLOAT max1 = (h1>h2) ? h1:h2;
                    FLOAT max2 = (h3>h4) ? h3:h4;
                    FLOAT maxh = (max1>max2) ? max1:max2;

                    if (abs(maxh-minh) > cross_diff) {
                        zone->sklt  = ywd->AltCollSubSklt;
                        zone->mid_y = maxh;
                        cheated = TRUE;
                    };
                };
                if (!cheated) {
                    /*** Standard-Behandlung ***/
                    pool++->y = nw_sec->Height;
                    pool++->y = north_sec->Height;
                    pool++->y = this_sec->Height;
                    pool++->y = west_sec->Height;
                    pool->y   = this_sec->AvrgHeight;

                    /*** und Ebenenparameter berechnen... ***/
                    yw_GetABCD(zone->sklt, 0);
                    yw_GetABCD(zone->sklt, 1);
                    yw_GetABCD(zone->sklt, 2);
                    yw_GetABCD(zone->sklt, 3);
                };
            };
            break;
    };

    /*** fertig ***/
}

/*-----------------------------------------------------------------*/
_dispatcher(void, yw_YWM_INTERSECT, struct intersect_msg *msg)
/*
**  FUNCTION
**      Master Of Collisions.
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      04-Jul-95   floh    created
**      06-Jul-95   floh    debugging
**      21-Aug-95   floh    Jetzt nicht mehr nur zwei Fälle, sondern
**                          drei. Für Randsektoren, die
**                          angeschnitten werden.
**      20-May-96   floh    nach kleineren Problemen mit zu langen
**                          Vektoren (Endpunkte außerhalb Welt)
**                          mache ich jetzt erstmal einen kleinen
**                          Test [len(x->z)] des übergebenen
**                          Vektors -> falls der Test anschlägt, wird
**                          eine Warnung ausgegeben und ohne Intersection
**                          zurückgekehrt.
**      10-Jan-96   floh    Warnungs-LogMsg (len > 1000.0) entfernt.
**      24-Feb-98   floh    + Kollissions-Cheat-Handling
*/
{
    struct ypaworld_data *ywd = INST_DATA(cl,o);
    struct yw_CollisionZone zone[4];
    ULONG num_zones, i;
    LONG start_subx, start_suby;
    LONG end_subx, end_suby;

    FLOAT sx = msg->pnt.x;
    FLOAT sy = msg->pnt.y;
    FLOAT sz = msg->pnt.z;

    FLOAT ex = sx + msg->vec.x;
    FLOAT ez = sz + msg->vec.z;

    FLOAT len;

    /*** msg initialisieren ***/
    msg->t      = 2.0;
    msg->insect = FALSE;

    /*** Validity-Check ***/
    len = nc_sqrt(msg->vec.x*msg->vec.x + msg->vec.z*msg->vec.z);

    /*** Berechne Sub-Koordinaten für Start- und End-Punkt ***/
    start_subx = GET_SUB(sx);
    start_suby = GET_SUB(-sz);
    end_subx   = GET_SUB(ex);
    end_suby   = GET_SUB(-ez);

    /*** Fall-Unterscheidung ***/
    if ((start_subx == end_subx) && (start_suby == end_suby)) {

        /*** merke: der SubSektor, auf dem sich ein Fahrzeug befindet, ***/
        /*** darf nie mit Kollisions-Cheat behandelt werden            ***/
        num_zones = 1;
        zone[0].type = CZT_INVALID;
        yw_GetSklt(ywd, start_subx, start_suby, start_subx, start_suby, &(zone[0]), msg->flags);

    } else if ((start_subx != end_subx) && (start_suby != end_suby)) {

        num_zones = 4;
        zone[0].type = CZT_INVALID;
        zone[1].type = CZT_INVALID;
        zone[2].type = CZT_INVALID;
        zone[3].type = CZT_INVALID;
        yw_GetSklt(ywd, start_subx, start_suby, start_subx, start_suby, &(zone[0]), msg->flags);
        yw_GetSklt(ywd, start_subx, start_suby, start_subx, end_suby, &(zone[1]), msg->flags);
        yw_GetSklt(ywd, start_subx, start_suby, end_subx, start_suby, &(zone[2]), msg->flags);
        yw_GetSklt(ywd, start_subx, start_suby, end_subx, end_suby, &(zone[3]), msg->flags);

    } else {

        num_zones = 2;
        zone[0].type = CZT_INVALID;
        zone[1].type = CZT_INVALID;
        yw_GetSklt(ywd, start_subx, start_suby, start_subx, start_suby, &(zone[0]), msg->flags);
        yw_GetSklt(ywd, start_subx, start_suby, end_subx, end_suby, &(zone[1]), msg->flags);
    };

    /*** jetzt alle Intersection-Tests in der Queue... ***/
    for (i=0; i<num_zones; i++) {

        if (zone[i].type != CZT_INVALID) {

            /*** falls Slurp-Sklt, dieses anpassen... ***/
            if (zone[i].type != CZT_LEGO) {
                yw_PrepareSlurpSklt(ywd, &(zone[i]));
            };
            msg->pnt.x = sx - zone[i].mid_x;
            msg->pnt.y = sy - zone[i].mid_y;
            msg->pnt.z = sz - zone[i].mid_z;

            /*** und der Intersection-Test! ***/
            yw_Intersector(msg, &(zone[i]));

            /*** falls bereits Intersection auftrat, abbrechen ***/
            if (msg->insect) break;
        };
    };

    /*** phew, endlich geschafft... ***/
}

/*-----------------------------------------------------------------*/
_dispatcher(void, yw_YWM_INTERSECT2, struct intersect_msg *msg)
/*
**  FUNCTION
**      Alternativer Intersector, nicht geeignet für Erdfahrzeuge,
**      kann dafür aber beliebig lange Vektoren abhandeln.
**      Bei Erdfahrzeugen tritt ab und zu mal ein 
**      Diagonal-Durchrutscher auf...
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      01-Jan-96   floh    Aah nei Gaar!
**      02-Jan-96   floh    Ha! Fehler! die Differenz-Adder haben
**                          durch die gegenseitige Division ihr
**                          Vorzeichen verloren.
**      10-Jan-96   floh    in INBACT2 wurde ein Rundungs-Fehler behoben,
**                          der sich auch in yw_YWM_INTERSECT2
**                          befunden haben dürfte.
**      24-Feb-98   floh    + Kollissions-Cheat-Handling
*/
{
    struct ypaworld_data *ywd = INST_DATA(cl,o);
    FLOAT sx,sy,sz;
    LONG ifx,ifz,idx,idz;
    FLOAT limit;
    FLOAT ax,az;

    struct yw_CollisionZone zone;
    LONG sub_x,sub_z;
    LONG start_sub_x, start_sub_z;

    /*** msg initialisieren ***/
    msg->t      = 2.0;
    msg->insect = FALSE;

    /*** Start-Punkt aufheben ***/
    sx = msg->pnt.x;
    sy = msg->pnt.y;
    sz = msg->pnt.z;

    /*** 4x Oversampled Sub-Koordinaten ermitteln ***/
    ifx = (LONG) (((sx+150.0) / 75.0) * 16384.0);
    ifz = (LONG) (((sz-150.0) / 75.0) * 16384.0);

    /*** Differenz-Adder ermitteln ***/
    ax = fabs(msg->vec.x);
    az = fabs(msg->vec.z);

    if ((ax == 0.0) && (az == 0.0)) {
        idx   = 0;
        idz   = 0;
        limit = 0;
    } else if (ax > az) {
        idx = (msg->vec.x < 0) ? (-1<<14):(1<<14);
        idz = (LONG) ((az*16384.0)/ax);
        if (msg->vec.z < 0.0) idz=-idz;
        limit = ax;
    } else {
        idx = (LONG) ((ax*16384.0)/az);
        if (msg->vec.x < 0.0) idx=-idx;
        idz = (msg->vec.z < 0) ? (-1<<14):(1<<14);
        limit = az;
    };

    start_sub_x = ifx>>16;
    start_sub_z = (-ifz)>>16;
    do {
        sub_x = ifx>>16;
        sub_z = (-ifz)>>16;
        zone.type = CZT_INVALID;

        yw_GetSklt(ywd,start_sub_x,start_sub_z,sub_x,sub_z,&zone,msg->flags);

        if (zone.type != CZT_INVALID) {
            if (zone.type != CZT_LEGO) yw_PrepareSlurpSklt(ywd,&zone);
            msg->pnt.x = sx - zone.mid_x;
            msg->pnt.y = sy - zone.mid_y;
            msg->pnt.z = sz - zone.mid_z;
            yw_Intersector(msg,&zone);
            if (msg->insect) return;
        };

        /*** ermittle nächstes gültiges ifx,ifz (mit Oversampling) ***/
        do { 
            ifx += idx; 
            ifz += idz;
            limit -= 75.0;
        } while ((((ifx>>16)==sub_x) && (((-ifz)>>16)==sub_z)) && (limit > 0));

    } while (limit > 0);

    /*** nochmal genau auf den Endpunkt, falls notwendig ***/
    ifx = (LONG) (((sx+150.0+msg->vec.x) / 75.0) * 16384.0);
    ifz = (LONG) (((sz-150.0+msg->vec.z) / 75.0) * 16384.0);

    if (((ifx>>16) != sub_x) || (((-ifz)>>16) != sub_z)) {

        sub_x = ifx>>16;
        sub_z = (-ifz)>>16;
        zone.type = CZT_INVALID;

        yw_GetSklt(ywd,start_sub_x,start_sub_z,sub_x,sub_z,&zone,msg->flags);

        if (zone.type != CZT_INVALID) {
            if (zone.type != CZT_LEGO) yw_PrepareSlurpSklt(ywd,&zone);
            msg->pnt.x = sx - zone.mid_x;
            msg->pnt.y = sy - zone.mid_y;
            msg->pnt.z = sz - zone.mid_z;
            yw_Intersector(msg,&zone);
        };
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
_dispatcher(void, yw_YWM_INSPHERE, struct insphere_msg *msg)
/*
**  FUNCTION
**      Testet, ob ausgehend von einem 3D-Punkt eine Intersection
**      mit der Umwelt innerhalb eines gegebenen Radius auftritt.
**      Das ist kein *exakter* Kugel-Intersection-Test, aber
**      in etwa damit vergleichbar. Zurückgegeben werden alle
**      festgestellten Intersections (siehe <struct insphere_msg>
**      und <struct Insect>.
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      06-Jul-95   floh    created
**      17-Jul-95   floh    Bugfix: in case 3 wurde tp_y statt tm_y
**                          übergeben, führte dazu, daß die Kollisions-
**                          Kontrolle in Richtung Süden nicht klappte.
**                          In case 5 war offensichtlich derselbe Bug :-/
*/
{
    struct ypaworld_data *ywd = INST_DATA(cl,o);
    struct yw_CollisionZone zone;
    LONG subx,suby;
    ULONG side_code;
    LONG tp_x,tp_y,tm_x,tm_y;

    FLOAT sx = msg->pnt.x;
    FLOAT sy = msg->pnt.y;
    FLOAT sz = msg->pnt.z;

    msg->num_insects = 0;

    /*** Sub-Koordinaten für Punkt-Zentrum... ***/
    subx = GET_SUB(sx);
    suby = GET_SUB(-sz);
    tm_x = GET_SUB(sx - msg->radius);
    tm_y = GET_SUB(-(sz - msg->radius));
    tp_x = GET_SUB(sx + msg->radius);
    tp_y = GET_SUB(-(sz + msg->radius));

    /*** jetzt 9 Fälle abhandeln... ***/
    for (side_code=0; side_code < 9; side_code++) {

        zone.type = CZT_INVALID;

        switch(side_code) {

            case 0:
                /*** Fall 0, Zentrum der Kugel... ***/
                yw_GetSklt(ywd, subx, suby, subx, suby, &zone, msg->flags);
                break;

            case 1:
                /*** Fall 1, ZentrumX - Radius ***/
                if (tm_x != subx) {
                    yw_GetSklt(ywd, subx, suby, tm_x, suby, &zone, msg->flags);
                };
                break;

            case 2:
                /*** Fall 2, ZentrumX + Radius ***/
                if (tp_x != subx) {
                    yw_GetSklt(ywd, subx, suby, tp_x, suby, &zone, msg->flags);
                };
                break;

            case 3:
                /*** Fall 3, ZentrumY - Radius ***/
                if (tm_y != suby) {
                    yw_GetSklt(ywd, subx, suby, subx, tm_y, &zone, msg->flags);
                };
                break;

            case 4:
                /*** Fall 4, ZentrumY + Radius ***/
                if (tp_y != suby) {
                    yw_GetSklt(ywd, subx, suby, subx, tp_y, &zone, msg->flags);
                };
                break;

            case 5:
                /*** Fall 5, ZX - R, ZY - R ***/
                if ((tm_x != subx) && (tm_y != suby)) {
                    yw_GetSklt(ywd, subx, suby, tm_x, tm_y, &zone, msg->flags);
                };
                break;

            case 6:
                /*** Fall 6, ZX + R, ZY - R ***/
                if ((tp_x != subx) && (tm_y != suby)) {
                    yw_GetSklt(ywd, subx, suby, tp_x, tm_y, &zone, msg->flags);
                };
                break;

            case 7:
                /*** Fall 7, ZX + R, ZY + R ***/
                if ((tp_x != subx) && (tp_y != suby)) {
                    yw_GetSklt(ywd, subx, suby, tp_x, tp_y, &zone, msg->flags);
                };
                break;

            case 8:
                /*** Fall 8, ZX - R, ZY + R ***/
                if ((tm_x != subx) && (tp_y != suby)) {
                    yw_GetSklt(ywd, subx, suby, tm_x, tp_y, &zone, msg->flags);
                };
                break;
        };

        /*** mit Zone intersecten... ***/
        if (zone.type != CZT_INVALID) {

            /* falls Slurp-Skeleton, dieses geometrisch anpassen... */
            if (zone.type != CZT_LEGO) {
                yw_PrepareSlurpSklt(ywd, &zone);
            };

            /* 3D-Punkt relativ zu Skeleton... */
            msg->pnt.x = sx - zone.mid_x;
            msg->pnt.y = sy - zone.mid_y;
            msg->pnt.z = sz - zone.mid_z;

            /* und endlich der Intersection-Check */
            yw_SphereIntersector(msg, &zone);
        };
    };

    /*** phew, endlich geschafft... ***/
}

/*-----------------------------------------------------------------*/
_dispatcher(void, yw_YWM_INBACT, struct inbact_msg *msg)
/*
**  FUNCTION
**      Testet, ob der Vektor ein Bakterium schneidet. Es
**      wird das Bakterium zurückgegeben, das dem Start-Punkt
**      am nächsten liegt. Der Vektor darf sich über mehrere
**      Sektoren erstrecken.
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      02-Jan-96   floh    created
**      03-Jan-96   floh    Oops, der Test war immer erfolgreich,
**                          wegen Intersection mit dem Bacterium,
**                          von dem der Check ausgeht... deshalb jetzt
**                          ein Bact-Pointer in Msg, der ignoriert
**                          werden soll...
**      13-Mar-96   floh    Bakterien im ACTION_DEAD-State werden
**                          nicht beachtet -> wenn ein Robo unter starkem
**                          Beschuß war, hatten die dran klebenden
**                          Raketen-Leichen eine Zielaufnahme verhindert.
**      07-Sep-96   floh    sqrt() ersetzt durch nc_sqrt()
**      12-Dec-97   floh    + falls Bact == Viewer und UserSitsInRoboFlak,
**                            wird der Robo ignoriert
*/
{
    struct ypaworld_data *ywd = INST_DATA(cl,o);
    LONG ifx,ifz,idx,idz;
    LONG limit;
    FLOAT ax,az;
    FLOAT vx,vy,vz;
    BOOL ignore_robo = ((msg->me==ywd->UVBact) && (ywd->UserSitsInRoboFlak));

    LONG sec_x,sec_y;

    vx = msg->vec.x;
    vy = msg->vec.y;
    vz = msg->vec.z;

    /*** msg unschädlich machen ***/
    msg->bhit = NULL;
    msg->dist = nc_sqrt(vx*vx+vy*vy+vz*vz);

    /*** 4x Oversampled Sektor-Koordinaten ***/
    ifx = (LONG) ((msg->pnt.x / 300.0) * 16384.0);
    ifz = (LONG) ((msg->pnt.z / 300.0) * 16384.0);

    /*** Differenz-Adder ermitteln ***/
    ax = fabs(vx);
    az = fabs(vz);

    if ((ax == 0.0) && (az == 0.0)) {
        idx = 0;
        idz = 0;
        limit = 0;
    } else if (ax > az) {
        idx = (vx < 0) ? (-1<<14):(1<<14);
        idz = (LONG) ((az*16384.0)/ax);
        if (vz < 0.0) idz = -idz;
        limit = (LONG) ax;
    } else {
        idx = (LONG) ((ax*16384.0)/az);
        if (vx < 0.0) idx = -idx;
        idz = (vz < 0.0) ? (-1<<14):(1<<14);
        limit = (LONG) az;
    };

    /*** der Ziel-Vektor kann jetzt normalisiert werden ***/
    vx /= msg->dist;
    vy /= msg->dist;
    vz /= msg->dist;

    do {
        sec_x     = ifx>>16;
        sec_y     = (-ifz)>>16;

        if ((sec_x >= 0) && (sec_x < ywd->MapSizeX) &&
            (sec_y >= 0) && (sec_y < ywd->MapSizeY))
        {
            struct Cell *sec = &(ywd->CellArea[sec_y * ywd->MapSizeX + sec_x]);
            struct MinList *ls;
            struct MinNode *nd;

            /*** für jede Bakterie im Sektor... ***/
            ls = &(sec->BactList);
            for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {

                struct Bacterium *b = (struct Bacterium *) nd;
                FLOAT tx,ty,tz;
                FLOAT vpx,vpy,vpz;

                /*** bin ich das selbst oder was totes (iiihh)? ***/
                if ((msg->me != b) && (b->MainState != ACTION_DEAD) && (!(ignore_robo && (b==ywd->URBact))))
                {
                    /*** Vektor auf aktuelle Bakterie ***/
                    tx = b->pos.x - msg->pnt.x;
                    ty = b->pos.y - msg->pnt.y;
                    tz = b->pos.z - msg->pnt.z;

                    /*** Vektor-Produkt auf Ziel- und Mouse-Vektor ***/
                    vpx = vy*tz - vz*ty;
                    vpy = vz*tx - vx*tz;
                    vpz = vx*ty - vy*tx;

                    /*** innerhalb Bact-Radius? ***/
                    if (nc_sqrt(vpx*vpx+vpy*vpy+vpz*vpz) < b->radius) {

                        FLOAT tdist;

                        /*** die Bakterie kann aber "hinter" uns sein! ***/
                        tdist = nc_sqrt(tx*tx+ty*ty+tz*tz);
                        if (((vx*tx + vy*ty + vz*tz)/tdist) > 0.0) {
                            /*** neue Bact näher als alte? ***/
                            tdist -= b->radius;
                            if (tdist < msg->dist) {

                                /*** Volltreffer! ***/
                                msg->dist = tdist;
                                msg->bhit = b;
                            };
                        };
                    };
                };
            };
        };

        /*** in diesem Sektor eine Bakterie gefunden? ***/
        if (msg->bhit) return;

        /*** nächste gültige Sektor-Koordinate ermitteln ***/
        do {
            ifx += idx;
            ifz += idz;
            limit -= 300;
        } while (((ifx>>16) == sec_x) && (((-ifz)>>16) == sec_y) && (limit>=0));

    } while (limit >= 0);

    /*** dös war's ***/
}

