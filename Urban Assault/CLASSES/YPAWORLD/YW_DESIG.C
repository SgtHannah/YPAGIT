/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_design.c,v $
**  $Revision: 38.15 $
**  $Date: 1998/01/06 16:17:58 $
**  $Locker: floh $
**  $Author: floh $
**
**  Initialisierung der Vehicle-, Building-, Weapon-Prototypes.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>
#include <utility/tagitem.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitmap/ilbmclass.h"
#include "ypa/ypavehicles.h"
#include "ypa/ypaworldclass.h"
#include "ypa/yparoboclass.h"

#include "yw_protos.h"

_extern_use_nucleus

/*-----------------------------------------------------------------*/
BOOL yw_InitPrototypeArrays(struct ypaworld_data *ywd)
/*
**  CHANGED
**      03-May-96   floh    created
*/
{
    /*** Prototype-Arrays allokieren und initialisieren ***/
    ywd->VP_Array = (struct VehicleProto *)
                    _AllocVec(NUM_VEHICLEPROTOS*sizeof(struct VehicleProto),
                              MEMF_PUBLIC|MEMF_CLEAR);
    ywd->WP_Array = (struct WeaponProto *)
                    _AllocVec(NUM_WEAPONPROTOS*sizeof(struct WeaponProto),
                              MEMF_PUBLIC|MEMF_CLEAR);
    ywd->BP_Array = (struct BuildProto *)
                    _AllocVec(NUM_BUILDPROTOS*sizeof(struct BuildProto),
                              MEMF_PUBLIC|MEMF_CLEAR);
    ywd->EXT_Array = (struct RoboExtension *)
                     _AllocVec(MAXNUM_DEFINEDROBOS*sizeof(struct RoboExtension),
                              MEMF_PUBLIC|MEMF_CLEAR);
    if (ywd->VP_Array && ywd->WP_Array && ywd->BP_Array && ywd->EXT_Array) {
        /*** alles klaa, Startup-Script parsen ***/
        if (!yw_ParseProtoScript(ywd, "data:scripts/startup.scr")) {
            return(FALSE);
        };
    } else return(FALSE);

    /*** alles klaaa ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void yw_CleanupPrototypeArrays(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Beräumt die Prototype-Arrays von irgendwelchen
**      herumlungernden Objekten, die evtl. beim Parsen
**      allokiert wurden.
**
**  CHANGED
**      24-Apr-97   floh    created
*/
{
    ULONG i;

    /*** VehicleProtos ***/
    if (ywd->VP_Array) {
        for (i=0; i<NUM_VEHICLEPROTOS; i++) {
            struct VehicleProto *vp = &(ywd->VP_Array[i]);
            if (vp->wireframe_object) {
                _dispose(vp->wireframe_object);
                vp->wireframe_object = NULL;
            };
        };
    };

    /*** WeaponProtos ***/
    if (ywd->WP_Array) {
        for (i=0; i<NUM_WEAPONPROTOS; i++) {
            struct WeaponProto *wp = &(ywd->WP_Array[i]);
            if (wp->wireframe_object) {
                _dispose(wp->wireframe_object);
                wp->wireframe_object = NULL;
            };
        };
    };
    
    /*** Buildprotos: nix aufzuräumen ***/
}

/*-----------------------------------------------------------------*/
void yw_KillPrototypeArrays(struct ypaworld_data *ywd)
/*
**  CHANGED
**      12-Apr-97   floh    created
*/
{
    yw_CleanupPrototypeArrays(ywd);
    if (ywd->EXT_Array) {
        _FreeVec(ywd->EXT_Array);
        ywd->EXT_Array = NULL;
    };
    if (ywd->BP_Array) {
        _FreeVec(ywd->BP_Array);
        ywd->BP_Array = NULL;
    };
    if (ywd->WP_Array) {
        _FreeVec(ywd->WP_Array);
        ywd->WP_Array = NULL;
    };
    if (ywd->VP_Array) {
        _FreeVec(ywd->VP_Array);
        ywd->VP_Array = NULL;
    };
}

/*=================================================================**
**                                                                 **
**  D E S I G N E R  S T U F F                                     **
**                                                                 **
**=================================================================*/


#ifdef YPA_DESIGNMODE
/*-----------------------------------------------------------------*/
BOOL yw_InitDesigner(struct ypaworld_data *ywd)
/*
**  CHANGED
**      21-Sep-96   floh    created
*/
{
    BOOL retval = FALSE;

    ywd->TypeMapName   = _AllocVec(128,MEMF_PUBLIC|MEMF_CLEAR);
    ywd->OwnerMapName  = _AllocVec(128,MEMF_PUBLIC|MEMF_CLEAR);
    ywd->BuildMapName  = _AllocVec(128,MEMF_PUBLIC|MEMF_CLEAR);
    ywd->HeightMapName = _AllocVec(128,MEMF_PUBLIC|MEMF_CLEAR);

    if ((ywd->TypeMapName) && (ywd->OwnerMapName) &&
        (ywd->BuildMapName) && (ywd->HeightMapName))
    {
        retval = TRUE;
    };

    return(retval);
}

/*-----------------------------------------------------------------*/
void yw_KillDesigner(struct ypaworld_data *ywd)
/*
**  CHANGED
**      21-Sep-96   floh    created
*/
{
    if (ywd->HeightMapName) _FreeVec(ywd->HeightMapName);
    if (ywd->BuildMapName)  _FreeVec(ywd->BuildMapName);
    if (ywd->OwnerMapName)  _FreeVec(ywd->OwnerMapName);
    if (ywd->TypeMapName)   _FreeVec(ywd->TypeMapName);

    ywd->HeightMapName = NULL;
    ywd->BuildMapName  = NULL;
    ywd->OwnerMapName  = NULL;
    ywd->TypeMapName   = NULL;
}

/*-----------------------------------------------------------------*/
void yw_DesignSetSector(struct ypaworld_data *ywd,
                        struct Cell *sec,
                        ULONG sec_x, ULONG sec_y,
                        ULONG sec_type)
/*
**  FUNCTION
**      Setzt einen Sektortyp im Designer-Modus neu.
**      Außerdem wird der Sektor-Typ in die Type-Map 
**      eingetragen.
**
**  INPUTS
**      ywd     - LID des Welt-Objects
**      sec     - Pointer auf Sektor in Sektor-Map
**      sec_x   - zusätzliche Koordinaten des Sektors
**      sec_y
**      sec_type    - der neue Sektor-Typ
**
**  CHANGED
**      23-May-96   floh    created
*/
{
    struct SectorDesc *sd = &(ywd->Sectors[sec_type]);
    struct VFMBitmap *blg_map, *typ_map;
    UBYTE *blg_body, *typ_body;
    ULONG lim,k,l;

    /*** Rand-Check ***/
    if ((sec_x == 0) || (sec_y == 0) ||
        (sec_x == (ywd->MapSizeX-1)) ||
        (sec_y == (ywd->MapSizeY-1)))
    {
        /*** ein RandSektor ***/
        return;
    };

    /*** Sektor initialisieren ***/
    sec->Type         = sec_type;
    sec->SType        = sd->SecType;

    /*** Anfangs-Energy initialisieren ***/
    if (sd->SecType == SECTYPE_COMPACT) {
        memset(sec->SubEnergy,0,sizeof(sec->SubEnergy));
        lim = 1;
    } else {
        lim = 3;
    };
    for (k=0; k<lim; k++) {
        for (l=0; l<lim; l++) {
            sec->SubEnergy[k][l] = sd->SSD[k][l]->init_energy;
        };
    };

    /*** den Sektor in der Build-Map löschen, in der TypeMap setzen ***/
    _get(ywd->BuildMap, BMA_Bitmap, &blg_map);
    _get(ywd->TypeMap,  BMA_Bitmap, &typ_map);
    blg_body = (UBYTE *) blg_map->Data;
    typ_body = (UBYTE *) typ_map->Data;

    blg_body[sec_y*blg_map->Width + sec_x] = 0;
    typ_body[sec_y*typ_map->Width + sec_x] = sec_type;
}

/*-----------------------------------------------------------------*/
void yw_DesignSetOwner(struct ypaworld_data *ywd,
                        struct Cell *sec,
                        ULONG sec_x, ULONG sec_y,
                        ULONG owner)
/*
**  FUNCTION
**      Setzt den Owner des Sektors im Design-Modus.
**
**  INPUTS
**      ywd     - LID des Welt-Objects
**      sec     - Pointer auf Sektor in Sektor-Map
**      sec_x   - zusätzliche Koordinaten des Sektors
**      sec_y
**      owner   - der Owner halt
**
**  CHANGED
**      23-May-96   floh    created
**      02-Nov-96   floh    + updated das SectorCount[] Array
*/
{
    struct VFMBitmap *own_map;
    UBYTE *own_body;

    /*** Rand-Check ***/
    if ((sec_x == 0) || (sec_y == 0) ||
        (sec_x == (ywd->MapSizeX-1)) ||
        (sec_y == (ywd->MapSizeY-1)))
    {
        /*** ein RandSektor ***/
        return;
    };

    /*** SectorCount updaten ***/
    ywd->SectorCount[sec->Owner]--;
    ywd->SectorCount[owner]++;

    /*** Sektor initialisieren ***/
    sec->Owner = owner;

    /*** den Sektor in der Owner-Map löschen, ***/
    _get(ywd->OwnerMap,  BMA_Bitmap, &own_map);
    own_body = (UBYTE *) own_map->Data;

    own_body[sec_y*own_map->Width+sec_x] = owner;
}

/*-----------------------------------------------------------------*/
void yw_DesignSetHeight(struct ypaworld_data *ywd,
                        struct Cell *sec,
                        ULONG sec_x, ULONG sec_y,
                        ULONG mode)
/*
**  FUNCTION
**      Ändert die Höhe des Sektors.
**
**  INPUTS
**      ywd     - LID des Welt-Objects
**      sec     - Pointer auf Sektor in Sektor-Map
**      sec_x   - zusätzliche Koordinaten des Sektors
**      sec_y
**      mode    - 0 -> Sektor wird abgesenkt,
**                1 -> Sektor wird angehoben
**
**  CHANGED
**      23-May-96   floh    created
*/
{
    struct VFMBitmap *hgt_map;
    UBYTE *hgt_body;
    LONG hgt,i,j;

    /*** Rand-Check ***/
    if ((sec_x == 0) || (sec_y == 0) ||
        (sec_x == (ywd->MapSizeX-1)) ||
        (sec_y == (ywd->MapSizeY-1)))
    {
        /*** ein RandSektor ***/
        return;
    };

    /*** hole die Original-Höhe aus der HeightMap ***/
    _get(ywd->HeightMap, BMA_Bitmap, &hgt_map);
    hgt_body = (UBYTE *) hgt_map->Data;

    /*** die Original-Höhe wird modifiziert!!! ***/
    hgt = hgt_body[sec_y*hgt_map->Width+sec_x];
    if (mode == 0) hgt--;
    else           hgt++;
    if (hgt < 0)        hgt = 0;
    else if (hgt > 255) hgt = 255;
    hgt_body[sec_y*hgt_map->Width+sec_x] = hgt;

    /*** "echte" Höhe in die Welt, und die AvrgHeight neu berechnen ***/
    sec->Height = (FLOAT) hgt * SECTOR_HEIGHTSCALE;

    /*** Durchschnitts-Höhen der näheren Umgebung neu bestimmen ***/
    for (i=sec_y; i<=(sec_y+1); i++) {
        for (j=sec_x; j<=(sec_x+1); j++) {

            LONG off1 = i*ywd->MapSizeX + j;
            LONG off2 = (i-1)*ywd->MapSizeX + (j-1);
            LONG off3 = (i-1)*ywd->MapSizeX + j;
            LONG off4 = i*ywd->MapSizeX + (j-1);

            struct Cell *s1 = &(ywd->CellArea[off1]);
            struct Cell *s2 = &(ywd->CellArea[off2]);
            struct Cell *s3 = &(ywd->CellArea[off3]);
            struct Cell *s4 = &(ywd->CellArea[off4]);

            s1->AvrgHeight = (s1->Height +
                              s2->Height +
                              s3->Height +
                              s4->Height)/4.0;
        };
    };
}

/*-----------------------------------------------------------------*/
void yw_DesignWriteMapBlock(struct ypaworld_data *ywd, APTR fp)
/*
**  FUNCTION
**      Schreibt den Map-Block as ASCII-Stream raus.
**
**  CHANGED
**      23-Jun-97   floh    created
*/
{
    fprintf(fp,"begin_maps\n");
    fprintf(fp,"; ------------------------------------------ \n");
    fprintf(fp,";--- machine generated map dumps, go away ---\n");
    fprintf(fp,"; ------------------------------------------ \n");

    /*** TypMap ***/
    fprintf(fp,"    typ_map =\n");
    yw_SaveBmpAsAscii(ywd,ywd->TypeMap,"        ",fp);

    /*** OwnMap ***/
    fprintf(fp,"    own_map =\n");
    yw_SaveBmpAsAscii(ywd,ywd->OwnerMap,"        ",fp);

    /*** HgtMap ***/
    fprintf(fp,"    hgt_map = \n");
    yw_SaveBmpAsAscii(ywd,ywd->HeightMap,"        ",fp);

    /*** BlgMap ***/
    fprintf(fp,"    blg_map = \n");
    yw_SaveBmpAsAscii(ywd,ywd->BuildMap,"        ",fp);

    fprintf(fp,"; ------------------------ \n");
    fprintf(fp,";--- map dumps end here ---\n");
    fprintf(fp,"; ------------------------ \n");
    fprintf(fp,"end\n");
}

/*-----------------------------------------------------------------*/
BOOL yw_DesignSaveMaps(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Sichert die einzelnen Maps in levels/maps unter
**          new.typ
**          new.own
**          new.hgt
**          new.blg
**
**  CHANGED
**      24-May-96   floh    created
**      21-Sep-96   floh    sichert jetzt die Maps unter dem selben
**                          Namen wie geladen
**      14-Oct-96   floh    alle Maps werden jetzt mit einer
**                          Graustufen-Colormap abgespeichert
**      23-Jun-97   floh    + Support für All-In-One-LDF-Maps.
**      24-Jun-97   floh    + remove()/rename() noch eingebaut
*/
{
    BOOL retval = FALSE;
    UBYTE *old_ldf;
    UBYTE *new_ldf;
    UBYTE new_buf[256];
    UBYTE old_buf[256];
    APTR old_fp,new_fp;
    LONG error;

    _ManglePath("data:scripts/dsgn.ldf",new_buf,sizeof(new_buf));
    _ManglePath(ywd->LevelNet->Levels[ywd->Level->Num].ldf,old_buf,sizeof(old_buf));
    new_ldf = new_buf;
    old_ldf = old_buf;
    old_fp = _FOpen(old_ldf,"r");
    new_fp = _FOpen(new_ldf,"w");
    if (old_fp && new_fp) {

        UBYTE line[1024];
        UBYTE parse[1024];
        BOOL inside_map_block  = FALSE;
        BOOL map_block_written = FALSE;

        /*** kopiere Zeile für Zeile, mit Ausnahme-Behandlung ***/
        while (_FGetS(line,sizeof(line),old_fp)) {

            UBYTE *kw;

            /*** Original-Zeile zum Parsen kopieren ***/
            strcpy(parse,line);
            kw = strtok(parse," \n\t");
            if (kw) {
                if (inside_map_block) {
                    /*** wenn innerhalb Map-Block, einfach skippen bis "end" ***/
                    if (stricmp(kw,"end")==0) inside_map_block = FALSE;
                } else {
                    /*** irgendein "Schlüssel-Keyword?" ***/
                    if (stricmp(kw,"typ")==0){
                        /*** diese Zeile nicht kopieren ***/
                    }else if (stricmp(kw,"own")==0){
                        /*** diese Zeile nicht kopieren ***/
                    }else if (stricmp(kw,"hgt")==0){
                        /*** diese Zeile nicht kopieren ***/
                    }else if (stricmp(kw,"blg")==0){
                        /*** diese Zeile nicht kopieren ***/
                    }else if (stricmp(kw,"begin_maps")==0){
                        /*** schreibe einen neuen Map-Block ***/
                        yw_DesignWriteMapBlock(ywd,new_fp);
                        inside_map_block  = TRUE;
                        map_block_written = TRUE;
                    }else{
                        /*** normale Zeile -> kopieren ***/
                        fputs(line,new_fp);
                    };
                };
            }else{
                /*** eine Leerzeile "simulieren" ***/
                fputs("\n",new_fp);
            };
        };
        /*** hatte Original-File schon einen Map-Block ***/
        if (!map_block_written) yw_DesignWriteMapBlock(ywd,new_fp);
        retval = TRUE;
        _FClose(old_fp);
        _FClose(new_fp);
        error = remove(old_ldf);
        error = rename(new_ldf,old_ldf);
    };
    return(retval);
}

/*-----------------------------------------------------------------*/
BOOL yw_DesignSaveProfile(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Erzeugt eine Höhen-Profil-Map für den gesamten
**      Level. Die Map ist immer 1024x1024. Die "vertikale"
**      Auflösung wird automatisch aus dem höchsten und
**      niedrigsten Sektor in der Map ermittelt.
**
**  CHANGED
**      12-Oct-96   floh    created
**      11-Jul-97   floh    schreibt jetzt auch die Höhenmap
**                          extra raus.
*/
{
    Object *bmp_o;
    BOOL retval = FALSE;

    /*** 1024x1024 shared Bitmap erzeugen ***/
    bmp_o = _new("bitmap.class",
                 RSA_Name,   "profile_bmp",
                 BMA_Width,  1024,
                 BMA_Height, 1024,
                 BMA_HasColorMap, TRUE,
                 TAG_DONE);
    if (bmp_o) {

        LONG x,y,i;
        Object *ilbm_o;
        FLOAT x0,z0,x1,z1,dx,dz;
        struct VFMBitmap *bmp;
        UBYTE *body;
        UBYTE *cm;

        /*** ColorMap ausfüllen (0=schwarz, 255=weiß) ***/
        _get(bmp_o,BMA_Bitmap,&bmp);
        cm = bmp->ColorMap;
        for (i=0; i<256; i++) {
            *cm++ = i;
            *cm++ = i;
            *cm++ = i;
        };

        /*** Intersection-Grid über den gesamten Level ***/
        x0 = 0.3;   // verhindert ungültige Intersections
        z0 = -0.4;
        x1 = ywd->MapSizeX * SECTOR_SIZE + 0.3;
        z1 = -(ywd->MapSizeY * SECTOR_SIZE + 0.4);
        dx = (x1-x0) / 1024.0;
        dz = (z1-z0) / 1024.0;

        /*** 1024x1024 Intersections auf gesamte Welt ***/
        body = (UBYTE *) bmp->Data;
        for (y=0; y<1024; y++,z0+=dz) {
            for (x=0,x0=0.3; x<1024; x++,x0+=dx,body++) {

                LONG pix=0;
                struct intersect_msg im;
                LONG subx,suby;

                /*** sind wir auf einem Slurp? ***/
                subx = GET_SUB(x0);
                suby = GET_SUB(-z0);
                if (((subx&3)==0)||((suby&3)==0)) {
                    /*** ja, Slurp ***/
                    *body = 0;
                } else {
                    /*** kein Slurp, Intersection an aktuellem Punkt ***/
                    im.pnt.x = x0;
                    im.pnt.y = -50000.0;
                    im.pnt.z = z0;
                    im.vec.x = 0.0;
                    im.vec.y = 100000.0;
                    im.vec.z = 0.0;
                    im.flags = 0;
                    _methoda(ywd->world,YWM_INTERSECT,&im);
                    if (im.insect) {

                        LONG secx = subx>>2;
                        LONG secy = suby>>2;
                        struct Cell *sec;

                        sec = &(ywd->CellArea[secy*ywd->MapSizeX+secx]);
                        pix = (LONG) ((sec->Height-im.ipnt.y)*256.0)/1000.0;
                        if (pix<0)        pix=0;
                        else if (pix>255) pix=255;
                        *body = pix;
                    };
                };
            };
        };

        /*** Schatten-ILBM-Bitmap-Object erzeugen und absaven ***/
        ilbm_o = _new("ilbm.class",
                      RSA_Name, "profile_bmp",
                      ILBMA_SaveILBM, TRUE,
                      TAG_DONE);
        if (ilbm_o) {

            UBYTE path[128];
            UBYTE old_path[128];
            struct rsrc_save_msg rsm;

            /*** Resource-Pfad umbiegen ***/
            strcpy(old_path,_GetAssign("rsrc"));
            sprintf(path,"maps/prof_%02d.iff",ywd->Level->Num);
            _SetAssign("rsrc","data:");
            rsm.name = path;
            rsm.iff  = NULL;
            rsm.type = RSRC_SAVE_STANDALONE;
            _methoda(ilbm_o,RSM_SAVE,&rsm);
            _dispose(ilbm_o);

            /*** dasselbe für die Height Map ***/
            _get(ywd->HeightMap,BMA_Bitmap,&bmp);
            cm = bmp->ColorMap;
            for (i=0; i<256; i++) {
                *cm++ = i;
                *cm++ = i;
                *cm++ = i;
            };
            sprintf(path,"maps/hgt_%02d.iff",ywd->Level->Num);
            rsm.name = path;
            rsm.iff  = NULL;
            rsm.type = RSRC_SAVE_STANDALONE;
            _methoda(ywd->HeightMap,RSM_SAVE,&rsm);

            /*** Resource-Pfad zurück ***/
            _SetAssign("rsrc",old_path);
            retval = TRUE;
        };

        /*** Ursprungs-Bitmap-Object killen ***/
        _dispose(bmp_o);
    };

    /*** Ende ***/
    return(retval);
}
#endif

