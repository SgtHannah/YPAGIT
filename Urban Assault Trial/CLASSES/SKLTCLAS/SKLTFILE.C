/*
**  $Source: PRG:VFM/Classes/_SKLTClass/skltfile.c,v $
**  $Revision: 38.3 $
**  $Date: 1996/02/28 23:15:25 $
**  $Locker:  $
**  $Author: floh $
**
**  sklt.class Resource-File-IO.
**
**  (C) Copyright 1995,1996 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>
#include <libraries/iffparse.h>

#include <string.h>
#include <math.h>

#include "nucleus/nucleus2.h"
#include "nucleus/nukedos.h"

#include "skeleton/skltclass.h"

_extern_use_nucleus

/*-----------------------------------------------------------------*/
struct IFFHandle *sklt_OpenIff(UBYTE *name, ULONG how)
/*
**  FUNCTION
**      Öffnet den übergebenen Filename als IFF-Stream
**      und returniert bei Erfolg einen IFFHandle.
**
**  INPUTS
**      name    - Filename, relativ zu MC2resources: !!!
**      how     - IFFF_READ oder IFFF_WRITE
**
**  RESULTS
**      Pointer auf offenen IFFStream.
**
**  CHANGED
**      11-Jan-96   floh    created
*/
{
    static UBYTE filename[256];
    LONG error;
    struct IFFHandle *iff;
    UBYTE *dos_mode;
    UBYTE *path_prefix;

    if (how == IFFF_READ) dos_mode = "rb";
    else                  dos_mode = "wb";

    /* vollen Filenamen zusammenbasteln */
    path_prefix = _GetSysPath(SYSPATH_RESOURCES);
    strcpy(filename, path_prefix);
    strcat(filename, name);

    /* und los geht's */
    if (iff = _AllocIFF()) {

        iff->iff_Stream = (ULONG) _FOpen(filename, dos_mode);
        if (iff->iff_Stream) {

            _InitIFFasNucleus(iff);

            error = _OpenIFF(iff, how);
            if (error == 0) {

                /*** Success!!! ***/
                return(iff);
            };
            _FClose((APTR)iff->iff_Stream);
        };
        _FreeIFF(iff);
    };

    /*** Fehler ***/
    return(NULL);
}

/*-----------------------------------------------------------------*/
void sklt_CloseIff(struct IFFHandle *iff)
/*
**  FUNCTION
**      Schließt einen mit ilbm_OpenIff() geöffneten
**      IFF-Stream wieder. ilbm_CloseIff() darf ***nur***
**      aufgerufen werden, wenn ilbm_OpenIff() erfolgreich
**      war!
**
**  INPUTS
**      iff - Ptr auf IFFHandle
**
**  RESULTS
**      ---
**
**  CHANGED
**      11-Jan-96   floh    created
*/
{
    if (iff) {
        _CloseIFF(iff);
        _FClose((APTR)iff->iff_Stream);
        _FreeIFF(iff);
    };
}

/*-----------------------------------------------------------------*/
void sklt_RefreshPlane(UBYTE *skltname, struct Skeleton *sklt, ULONG pnum)
/*
**  FUNCTION
**      Berechnet die Ebenen-Parameter [A,B,C,D] des Polygons
**      <msg->pnum> neu. A,B,C entspricht dem Normalen-Vektor der Fläche.
**      Die Routine gibt es zwar auch als skeleton.class Methode,
**      diese ist aber aus der RSM_CREATE-Methode nicht
**      anwendbar, weil die LID des Objects noch nicht
**      initialisiert ist!
**
**  CHANGED
**      15-Jan-96   floh    created
*/
{
    if (sklt) {

        /*** hat Fläche mindestens 3 Punkte? ***/
        if (sklt->Areas[pnum][0] < 3) {
            sklt->PlanePool[pnum].A = 0.0;
            sklt->PlanePool[pnum].B = 0.0;
            sklt->PlanePool[pnum].C = 0.0;
            sklt->PlanePool[pnum].D = 0.0;
            return;

        } else {

            WORD p0,p1,p2;
            FLOAT ax,ay,az;
            FLOAT bx,by,bz;
            FLOAT A,B,C,D;
            FLOAT len;
            fp3d *pool = sklt->Pool;

            p0 = sklt->Areas[pnum][1];
            p1 = sklt->Areas[pnum][2];
            p2 = sklt->Areas[pnum][3];

            ax = pool[p1].x - pool[p0].x;
            ay = pool[p1].y - pool[p0].y;
            az = pool[p1].z - pool[p0].z;

            bx = pool[p2].x - pool[p1].x;
            by = pool[p2].y - pool[p1].y;
            bz = pool[p2].z - pool[p1].z;

            A = ay*bz - az*by;
            B = az*bx - ax*bz;
            C = ax*by - ay*bx;

            len = sqrt(A*A + B*B + C*C);

            if (len == 0.0) {
                /*** Fläche nicht plane ***/
                A = 0.0;
                B = 0.0;
                C = 0.0;
            } else {
                /*** A,B,C normalisieren ***/
                A /= len;
                B /= len;
                C /= len;
            };

            /*** D = 0 - (Ax + By + Cz) ***/
            D = -(A*pool[p0].x + B*pool[p0].y + C*pool[p0].z);
            sklt->PlanePool[pnum].A = A;
            sklt->PlanePool[pnum].B = B;
            sklt->PlanePool[pnum].C = C;
            sklt->PlanePool[pnum].D = D;
        };
    };
}


/*-----------------------------------------------------------------*/
struct RsrcNode *sklt_getPool(Object *o, Class *cl, 
                              struct TagItem *tlist,
                              struct IFFHandle *iff, 
                              WORD type)
/*
**  FUNCTION
**      Ermittelt zuerst die Anzahl der Punkte im POOL-Chunk,
**      erzeugt dann mit Hilfe der Superklasse eine Skeleton-Resource,
**      und füllt deren Punkt-Pool mit dem Inhalt des Chunks.
**
**  INPUTS
**      o
**      cl
**      iff
**      type    -   1 -> alter POOL Chunk
**                  2 -> neuer POOL Chunk
**
**  RESULTS
**      Pointer auf Resource-Node mit teilweise initialisiertem
**      Skeleton oder NULL bei Mißerfolg.
**
**  CHANGED
**      15-Jan-96   floh    created
*/
{
    struct RsrcNode *rnode;
    ULONG num_pnts;
    struct TagItem add_tags[2];

    struct ContextNode *cn = _CurrentChunk(iff);

    /*** Anzahl Punkte im Chunk ermitteln ***/
    switch(type) {
        case 1:
            num_pnts = cn->cn_Size/sizeof(struct SKLT_PoolType1Element);
            break;
        case 2:
            num_pnts = cn->cn_Size/sizeof(struct sklt_NewPoolPoint);
            break;
        default:
            return(NULL);
            break;
    };

    /*** Resource-Node erzeugen ***/
    add_tags[0].ti_Tag  = SKLA_NumPoints;
    add_tags[0].ti_Data = num_pnts;
    add_tags[1].ti_Tag  = TAG_MORE;
    add_tags[1].ti_Data = (ULONG) tlist;

    rnode = (struct RsrcNode *) _supermethoda(cl,o,RSM_CREATE,add_tags);

    if (rnode) {

        struct Skeleton *sklt = rnode->Handle;
        ULONG i;
        struct SKLT_PoolType1Element pt1e;
        struct sklt_NewPoolPoint npp;

        /*** POOL Chunk reinlesen ***/
        switch(type) {

            case 1:
                for (i=0; i<num_pnts; i++) {

                    _ReadChunkBytes(iff,&pt1e,sizeof(pt1e));

                    /* Endian-Konvertierung + Pool füllen */
                    v2nw(&(pt1e.x));
                    v2nw(&(pt1e.y));
                    v2nw(&(pt1e.z));

                    sklt->Pool[i].flags = 0;
                    sklt->Pool[i].x = (FLOAT) pt1e.x;
                    sklt->Pool[i].y = (FLOAT) pt1e.y;
                    sklt->Pool[i].z = (FLOAT) pt1e.z;
                };
                break;

            case 2:
                for (i=0; i<num_pnts; i++) {

                    _ReadChunkBytes(iff,&npp,sizeof(npp));

                    /* Endian-Konvertierung + Pool füllen */
                    v2nl(&(npp.x));
                    v2nl(&(npp.y));
                    v2nl(&(npp.z));

                    sklt->Pool[i].flags = 0;
                    sklt->Pool[i].x = npp.x;
                    sklt->Pool[i].y = npp.y;
                    sklt->Pool[i].z = npp.z;
                };
                break;
        };
    };

    /*** kann NULL sein! ***/
    return(rnode);
}

/*-----------------------------------------------------------------*/
BOOL sklt_getOldPolys(Object *o, struct IFFHandle *iff,
                      struct Skeleton *sklt)
/*
**  FUNCTION
**      Polygon-Reader für das alte POLY-Chunk-Format.
**
**  CHANGED
**      15-Jan-96   floh    created
*/
{
    WORD *temp;
    struct ContextNode *cn = _CurrentChunk(iff);
    struct create_polygons_msg cp_msg;
    ULONG i;
    ULONG num_polys;

    /*** Fuck... um die Polygon-Anzahl zu ermitteln, muß ich den ***/
    /*** Chunk wohl oder übel erstmal in ein temporäres Array    ***/
    /*** laden                                                   ***/
    temp = (WORD *) _AllocVec(cn->cn_Size, MEMF_PUBLIC);
    if (!temp) return(FALSE);

    /*** altes Chunk-Format einlesen ***/
    _ReadChunkBytes(iff,temp,cn->cn_Size);

    /*** Anzahl Flächen im Temp-Buffer ermitteln, Endian-Konvertierung ***/
    num_polys = 0;
    for (i=0; i<(cn->cn_Size/sizeof(WORD)); i++) {
        v2nw(&(temp[i]));
        if (temp[i] == -1) num_polys++;
    };

    /*** Polygon-Arrays allokieren lassen ***/
    cp_msg.sklt         = sklt;
    cp_msg.num_polys    = num_polys;
    cp_msg.num_polypnts = (cn->cn_Size/sizeof(WORD)) - num_polys;
    if (_methoda(o, SKLM_CREATE_POLYGONS, &cp_msg)) {

        /*** Temp -> PolyPool ***/
        WORD *target = sklt->Areas[0];
        WORD *source = temp;

        for (i=0; i<num_polys; i++) {

            WORD n = 0;

            /*** Ptr auf aktuellen Polygon eintragen ***/
            sklt->Areas[i] = target;

            /*** je 1 Polygon ***/
            while (*source != -1) {
                target[n+1] = *source++;
                n++;
            };

            /*** Endekennung in Source überspringen ***/
            source++;

            /*** Anzahl Eckpunkte als 1.Word des Polys ***/
            target[0] = n;
            target += (n+1);
        };
    } else return(FALSE);

    /*** Temp-Buffer noch freigeben ***/
    _FreeVec(temp);

    /*** und zurück ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
BOOL sklt_getPolys(Object *o, struct IFFHandle *iff,
                   struct Skeleton *sklt)
/*
**  FUNCTION
**      Polygon-Reader für das neue NPOL-Chunk-Format.
**
**  CHANGED
**      15-Jan-96   floh    created
*/
{
    struct ContextNode *cn = _CurrentChunk(iff);
    struct create_polygons_msg cp_msg;
    ULONG num_polys;
    ULONG num_polypnts;
    ULONG i;

    /*** lese Anzahl Polygone ***/
    _ReadChunkBytes(iff, &num_polys, sizeof(num_polys));
    v2nl(&num_polys);

    /*** ermittle Anzahl Polypoints ***/
    num_polypnts = ((cn->cn_Size - 4) / sizeof(WORD)) - num_polys;

    cp_msg.sklt         = sklt;
    cp_msg.num_polys    = num_polys;
    cp_msg.num_polypnts = num_polypnts;
    if (_methoda(o, SKLM_CREATE_POLYGONS, &cp_msg)) {

        WORD *poly = sklt->Areas[0];

        /*** lese Chunk in Poly-Puffer ***/
        _ReadChunkBytes(iff, sklt->Areas[0], (cn->cn_Size-4));

        /*** Poly-Pointer ermitteln und Endian-Konvertierung ***/
        for (i=0; i<num_polys; i++) {

            WORD edges;
            ULONG j;

            sklt->Areas[i] = poly;

            /*** Eckpunkt-Anzahl dieses Polys ***/
            v2nw(poly);
            edges = *poly++;
            for (j=0; j<edges; j++) {
                v2nw(poly);
                poly++;
            };
        };
    } else return(FALSE);

    /*** und zurück ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
BOOL sklt_getSensorPool(Object *o, struct IFFHandle *iff,
                        struct Skeleton *sklt,
                        WORD type)
/*
**  FUNCTION
**      Erzeugt und initialisiert Sensor-Pool aus dem alten
**      SENS- oder dem neuen SEN2-Chunk-Format.
**
**  INPUTS
**      ...
**      type    1 -> altes SENS-Format
**              2 -> neues SEN2-Format
**
**  RESULTS
**      TRUE/FALSE
**
**  CHANGED
**      16-Jan-96   floh    created
*/
{
    ULONG num_pnts;
    struct create_sensorpool_msg csp_msg;

    struct ContextNode *cn = _CurrentChunk(iff);

    /*** Anzahl Punkte im Chunk ermitteln ***/
    switch(type) {
        case 1:
            num_pnts = cn->cn_Size/sizeof(struct SKLT_PoolType1Element);
            break;
        case 2:
            num_pnts = cn->cn_Size/sizeof(struct sklt_NewPoolPoint);
            break;
        default:
            return(NULL);
            break;
    };

    /*** Sensor-Pool erzeugen ***/
    csp_msg.sklt = sklt;
    csp_msg.num_sensorpnts = num_pnts;
    if (_methoda(o,SKLM_CREATE_SENSORPOOL,&csp_msg)) {

        ULONG i;
        struct SKLT_PoolType1Element pt1e;
        struct sklt_NewPoolPoint npp;

        /*** POOL Chunk reinlesen ***/
        switch(type) {

            case 1:
                for (i=0; i<num_pnts; i++) {

                    _ReadChunkBytes(iff,&pt1e,sizeof(pt1e));

                    /* Endian-Konvertierung + Pool füllen */
                    v2nw(&(pt1e.x));
                    v2nw(&(pt1e.y));
                    v2nw(&(pt1e.z));

                    sklt->SensorPool[i].flags = 0;
                    sklt->SensorPool[i].x = (FLOAT) pt1e.x;
                    sklt->SensorPool[i].y = (FLOAT) pt1e.y;
                    sklt->SensorPool[i].z = (FLOAT) pt1e.z;
                };
                break;

            case 2:
                for (i=0; i<num_pnts; i++) {

                    _ReadChunkBytes(iff,&npp,sizeof(npp));

                    /* Endian-Konvertierung + Pool füllen */
                    v2nl(&(npp.x));
                    v2nl(&(npp.y));
                    v2nl(&(npp.z));

                    sklt->SensorPool[i].flags = 0;
                    sklt->SensorPool[i].x = npp.x;
                    sklt->SensorPool[i].y = npp.y;
                    sklt->SensorPool[i].z = npp.z;
                };
                break;
        };
    } else return(FALSE);

    /*** alles OK ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
struct RsrcNode *sklt_CreateSkeleton(Object *o, Class *cl,
                                     struct TagItem *tlist,
                                     struct IFFHandle *iff)
/*
**  FUNCTION
**      Erzeugt aus einem IFF SKLT File eine Resource-Node
**      mit "eingebetteter" Skeleton-Resource. Die Routine
**      ***MUSS*** aus dem RSM_CREATE Handler der sklt.class
**      aufgerufen werden, weil sie selbständig RSM_CREATE der
**      Superklasse ausführt!
**
**  INPUTS
**      o       - wie bei RSM_CREATE reingekommen
**      cl      - wie bei RSM_CREATE reingekommen
**      tlist   - Message von RSM_CREATE
**      iff     - offener IFFHandle, gestoppt auf
**                äußerstem FORM des zu lesenden Files
**
**  RESULTS
**      Eine vollständige RsrcNode, oder NULL bei einem Fehler.
**
**  CHANGED
**      15-Jan-96   floh    created
**      16-Jan-96   floh    debugging...
*/
{
    LONG error;
    BOOL all_ok = TRUE;
    struct ContextNode *cn;
    struct RsrcNode *rnode = NULL;
    struct Skeleton *sklt = NULL;

    /*** die folgende Schleife akzeptiert sowohl das alte   ***/
    /*** "FORM MC2 FORM SKLT" Format, als auch das neue     ***/
    /*** "FORM SKLT" Format...                              ***/

    while ((error = _ParseIFF(iff, IFFPARSE_RAWSTEP)) != IFFERR_EOC) {
        /*** ander Fehler als EndOfChunk abfangen ***/
        if (error) {
            /*** falls RsrcNode bereits erzeugt, diese killen ***/
            if (rnode) _method(o, RSM_FREE, (ULONG) rnode);
            return(NULL);
        };

        /*** ContextNode holen ***/
        cn = _CurrentChunk(iff);

        /*** FORM-Chunks? -> generell reinsteppen ***/
        if (cn->cn_ID == ID_FORM) continue;

        /*** ignoriere Type-Chunk ***/
        if (cn->cn_ID == SKLTIFF_TYPE) {
            _ParseIFF(iff, IFFPARSE_RAWSTEP);
            continue;
        };

        /*** alter POOL Chunk? ***/
        if (cn->cn_ID == SKLTIFF_POOL) {
            rnode = sklt_getPool(o,cl,tlist,iff,1);
            if (rnode) {
                sklt = rnode->Handle;
                if (!sklt) all_ok = FALSE;
            } else all_ok = FALSE;
            _ParseIFF(iff, IFFPARSE_RAWSTEP);
            continue;
        };

        /*** NEUER POOL Chunk? ***/
        if (cn->cn_ID == SKLTIFF_NEWPOOL) {
            rnode = sklt_getPool(o,cl,tlist,iff,2);
            if (rnode) {
                sklt = rnode->Handle;
                if (!sklt) all_ok = FALSE;
            } else all_ok = FALSE;
            _ParseIFF(iff, IFFPARSE_RAWSTEP);
            continue;
        };

        /*** alter POLY Chunk? ***/
        if (cn->cn_ID == SKLTIFF_POLY) {
            if (sklt) {
                if (!sklt_getOldPolys(o,iff,sklt)) all_ok = FALSE;
            };
            _ParseIFF(iff, IFFPARSE_RAWSTEP);
            continue;
        };

        /*** neuer NPOL Chunk? ***/
        if (cn->cn_ID == SKLTIFF_NEWPOLY) {
            if (sklt) {
                if (!sklt_getPolys(o,iff,sklt)) all_ok = FALSE;
            };
            _ParseIFF(iff, IFFPARSE_RAWSTEP);
            continue;
        };

        /*** alter SENSOR Chunk? ***/
        if (cn->cn_ID == SKLTIFF_SENSOR) {
            if (sklt) {
                if (!sklt_getSensorPool(o,iff,sklt,1)) all_ok = FALSE;
            };
            _ParseIFF(iff, IFFPARSE_RAWSTEP);
            continue;
        };

        /*** neuer SENSOR-Chunk? ***/
        if (cn->cn_ID == SKLTIFF_NEWSENSOR) {
            if (sklt) {
                if (!sklt_getSensorPool(o,iff,sklt,2)) all_ok = FALSE;
            };
            _ParseIFF(iff, IFFPARSE_RAWSTEP);
            continue;
        };

        /*** unbekannte Chunks überspringen ***/
        _SkipChunk(iff);
    };

    /*** trat irgendwo ein Fehler auf? ***/
    if ((!all_ok) || (!sklt)) {
        if (rnode) _method(o, RSM_FREE, (ULONG) rnode);
        return(NULL);
    } else {
        /*** PlanePool on the fly füllen ***/
        ULONG i;
        for (i=0; i<sklt->NumAreas; i++) {
            sklt_RefreshPlane(rnode->Node.ln_Name,sklt,i);
        };
        return(rnode);
    };
}

