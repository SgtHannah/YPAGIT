/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_radar.c,v $
**  $Revision: 38.4 $
**  $Date: 1998/01/06 16:26:18 $
**  $Locker:  $
**  $Author: floh $
**
**  Funktionen für das neue Radar (rotierend und Hires-tauglich).
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <math.h>

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "bitmap/rasterclass.h"

#include "ypa/ypaworldclass.h"
#include "ypa/guimap.h"

#include "yw_protos.h"

/*** aus windd.class ***/
extern unsigned long wdd_DoDirect3D;

/*-----------------------------------------------------------------*/
_extern_use_nucleus

extern struct YPAMapReq MR;     // aus yw_mapreq.c

UBYTE RadarStr[8192];

/*-----------------------------------------------------------------*/
void yw_RadarGet2DNormalizedViewer(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Erzeugt eine normalisierte 2D-Matrix der X- und
**      Z-Viewer-Komponente, welche nach MR.r_dir
**      geschrieben wird. Bitte beachten, daß nur
**      m11,m13,m31 und m33 gültig sind!
**
**  CHANGED
**      15-Sep-96   floh    created
**      16-Sep-96   floh    Bugfixed.
*/
{
    FLOAT x,z,l;
    x = ywd->ViewerDir.m31;
    z = ywd->ViewerDir.m33;
    l = nc_sqrt(x*x+z*z);
    if (l>0) { x/= l;   z/= l; }
    else     { x = 0.0; z = 1.0; };
    MR.r_dir.m11 = z;
    MR.r_dir.m13 = -x;
    MR.r_dir.m31 = x;
    MR.r_dir.m33 = z;
}

/*-----------------------------------------------------------------*/
void yw_RenderRadar(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Rendert Radar. Muß aufgerufen werden, bevor die
**      anderen Requester layoutet und gezeichnet werden!
**
**  CHANGED
**      13-Sep-96   floh    created
**      15-Oct-97   floh    das Bitmap-Radar-Rendering der DirectDraw
**                          Version ist rausgeflogen
*/
{
    FLOAT dsp_width,dsp_height;

    /*** patche einige MR-Parameter ***/
    FLOAT old_midx   = MR.midx;
    FLOAT old_midz   = MR.midz;
    FLOAT old_x_asp  = MR.x_aspect;
    FLOAT old_y_asp  = MR.y_aspect;
    UBYTE old_layers = MR.layers;
    BYTE old_zoom    = MR.zoom;
    UBYTE old_fprint = MR.footprint;

    /*** Breite und Höhe in Pixel auf Screen ***/
    dsp_width  = ((FLOAT)(ywd->DspXRes>>1))*(MR.r_x1-MR.r_x0);
    dsp_height = ((FLOAT)(ywd->DspYRes>>1))*(MR.r_y1-MR.r_y0);
    MR.x_aspect = (4*SECTOR_SIZE) / dsp_width;
    MR.y_aspect = (4*SECTOR_SIZE) / dsp_height;
    MR.midx = ywd->Viewer->pos.x;
    MR.midz = ywd->Viewer->pos.z;
    MR.layers     = MAP_LAYER_LANDSCAPE;
    MR.zoom       = 3;
    MR.footprint  = 0xff;

    // FIXME_FLOH: CheckMidPoint würde zu Verschiebungen am
    // Levelrand führen, und ist in diesem speziellen
    // Fall nicht notwendig
    // yw_CheckMidPoint(ywd,dsp_width,dsp_height);

    /*** Radar-Flag für die Render-Routinen ***/
    MR.flags |= MAPF_RADAR_MODE;

    /*** 2D-normalisierte Pseudo-Viewer-Matrix erzeugen ***/
    yw_RadarGet2DNormalizedViewer(ywd);

    /*** Vehikel rendern ***/
    yw_RenderRadarVehicles(ywd);

    /*** Radar-Flag wieder deaktivieren ***/
    MR.flags &= ~MAPF_RADAR_MODE;

    /*** Original-Map-Parameter zurückpatchen ***/
    MR.midx = old_midx;
    MR.midz = old_midz;
    MR.x_aspect = old_x_asp;
    MR.y_aspect = old_y_asp;
    MR.layers     = old_layers;
    MR.zoom       = old_zoom;
    MR.footprint  = old_fprint;
}

