/*
**  $Source: PRG:VFM/Nucleus2/pc/system.c,v $
**  $Revision: 38.10 $
**  $Date: 1998/01/06 12:46:17 $
**  $Locker:  $
**  $Author: floh $
**
**  Systemspezifische Init/Cleanup-Routinen für Nucleus.
**
**  (C) Copyright 1994 by A.Weissflog
*/
#include <exec/types.h>

#include <float.h>

#include "nucleus/nucleus2.h"

/*-------------------------------------------------------------------
**  Die Segment-Liste aus dem segment.c Modul.
*/
extern struct NucleusBase NBase;
extern struct MinList SegmentList;

/*-------------------------------------------------------------------
**  Externe Codesegmente, die statisch eingelinkt werden
**  sollen.
*/

#ifndef NO_VFM_CLASSES
    extern struct SegInfo nucleus_class_seg;
    extern struct SegInfo rsrc_class_seg;
    extern struct SegInfo bitmap_class_seg;
    extern struct SegInfo skeleton_class_seg;
    extern struct SegInfo ilbm_class_seg;
    extern struct SegInfo sklt_class_seg;
    extern struct SegInfo ade_class_seg;
    extern struct SegInfo area_class_seg;
    extern struct SegInfo base_class_seg;
    extern struct SegInfo bmpanim_class_seg;
    extern struct SegInfo amesh_class_seg;
    extern struct SegInfo prtl_class_seg;
    extern struct SegInfo ebd_class_seg;
    extern struct SegInfo idev_class_seg;
    extern struct SegInfo inp_class_seg;
    extern struct SegInfo itimer_class_seg;
    extern struct SegInfo iwimp_class_seg;
    extern struct SegInfo sample_class_seg;
    extern struct SegInfo wav_class_seg;
    extern struct SegInfo rst_class_seg;
    extern struct SegInfo disp_class_seg;
    extern struct SegInfo bt_class_seg;
    extern struct SegInfo rq_class_seg;
    #ifdef __NETWORK__
    extern struct SegInfo nw_class_seg;
    #endif

    #ifdef __WINDOWS__
        /*** Windows Only ***/
        extern struct SegInfo windd_class_seg;
        extern struct SegInfo win3d_class_seg;
        extern struct SegInfo winp_class_seg;
        extern struct SegInfo wit_class_seg;
        #ifdef __NETWORK__
        extern struct SegInfo windp_class_seg;
        #endif
    #endif
    #ifdef __DOS__
        /*** MS-DOS-Only ***/
        extern struct SegInfo vga_class_seg;
        extern struct SegInfo milestimer_class_seg;
        extern struct SegInfo dmouse_class_seg;
        extern struct SegInfo dkey_class_seg;
        extern struct SegInfo dtimer_class_seg;

        #ifdef VFM_3DBLASTER
            /*** MS-DOS-&-3D-Blaster-Only ***/
            extern struct SegInfo cgl_class_seg;
        #endif
    #endif
#endif

#ifndef NO_VFM_ENGINES
    extern struct SegInfo tform_ng_engine_seg;
    extern struct SegInfo ie_engine_seg;
    extern struct SegInfo ove_engine_seg;
    extern struct SegInfo audio_miles_engine_seg;
#endif

#ifndef NO_YPA
    extern struct SegInfo yw_class_seg;     // ypaworld.class
    extern struct SegInfo yb_class_seg;     // ypabact.class
    extern struct SegInfo yt_class_seg;     // ypatank.class
    extern struct SegInfo yr_class_seg;     // yparobo.class
    extern struct SegInfo ym_class_seg;     // ypamissile.class
    extern struct SegInfo yf_class_seg;     // ypaflyer.class
    extern struct SegInfo yc_class_seg;     // ypacar.class
    extern struct SegInfo yu_class_seg;     // ypaufo.class
    extern struct SegInfo yg_class_seg;     // ypagun.class
#endif

/*-----------------------------------------------------------------*/
BOOL nc_SystemInit(void)
/*
**  FUNCTION
**      Alle statisch gelinkten Code-Segmente werden hier per
**      nc_AddSegment() in die interne Segment-Liste gelinkt,
**      vorher wird die Segment-Liste initialisiert.
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      28-Dec-94   floh    created
**      01-Jan-95   floh    nucleus_class_seg
**      02-Jan-95   floh    (1) tform_ng_engine_seg
**                          (2) resource_class_seg
**      03-Jan-95   floh    (1) bitmap_class_seg
**                          (2) skeleton_class_seg
**                          (3) ilbm_class_seg
**      05-Jan-95   floh    ove_vga_engine_seg
**      15-Jan-95   floh    (1) sklt_class_seg
**                          (2) ade_class_seg
**      18-Jan-95   floh    (1) area_class_seg
**      20-Jan-95   floh    base_class_seg
**      21-Jan-95   floh    bmpanim_class_seg
**      29-Jan-95   floh    ie_ibm_engine_seg
**      16-Mar-95   floh    amesh_class_seg
**      13-May-95   floh    yw_class_seg
**      13-Jun-95   floh    yb_class_seg
**      28-Jul-95   floh    yt_class_seg
**      21-Aug-95   floh    yz_class_seg
**      16-Sep-95   floh    prtl_class_seg
**      07-Oct-95   floh    + yr_class_seg
**                          + ym_class_seg
**      09-Nov-95   floh    + ya_class_seg
**      02-Dec-95   floh    + yf_class_seg
**      21-Dec-95   floh    - ya_class_seg
**      10-Jan-96   floh    + yc_class_seg
**                          + yu_class_seg
**      20-Jan-96   floh    - resource_class_seg
**                          + rsrc_class_seg
**      23-Jan-96   floh    + ebd_class_seg
**      04-Feb-96   floh    + yg_class_seg
**      03-Mar-96   floh    - ie_ibmpc_engine_seg
**                          + dmouse_class_seg
**                          + idev_class_seg
**                          + inp_class_seg
**                          + itimer_class_seg
**                          + iwimp_class_seg
**                          + ie_engine_seg
**      10-Mar-96   floh    + dkey_class_seg
**      11-Mar-96   floh    + dtimer_class_seg
**                          + yh_class_seg (AF's Update)
**      12-Mar-96   floh    + dbgm_class_seg
**      17-Mar-96   floh    - yz_class_seg (ypazepp.class)
**      17-Apr-96   floh    + audio_miles_engine_seg
**      18-Apr-96   floh    + sample_class_seg
**                          + wav_class_seg
**      19-Apr-96   floh    + Amok-Zeugs in #ifdefs eingebettet.
**                          + milestimer_class_seg
**      26-Apr-96   floh    + Initialisierung __AllocCount, __FOpenCount
**      29-Apr-96   floh    + bt_class_seg
**                          + rq_class_seg
**      09-Jun-96   floh    + rst_class_seg (raster.class)
**                          + disp_class_seg (display.class)
**                          + vga_class_seg (drivers/gfx/vga.class)
**                          - ove_vga_engine_seg
**                          + ove_engine_seg
**      04-Aug-96   floh    + __AllocSize, __AllocMax
**      13-Aug-96   floh    + experimentell: x86 FPU wird in
**                            24-Bit-Modus geschaltet
**      17-Aug-96   floh    + cgl.class (nur, wenn VFM_3DBLASTER definiert!)
**      23-Sep-96   floh    - ypahovercraft.class
**      10-Nov-96   floh    + windd.class
**      21-Nov-96   floh    + winp.class
**                          + wintimer.class
**      22-Nov-96   floh    + audio_nosound_engine_seg
**      26-Nov-96   floh    - dbgm_class_seg
**      19-Feb-97   floh    + audio_miles_engine_seg auch für Windows
**      24-Feb-97   floh    + diverse Diagnose-Variablen in NBase eingebettet
**      06-Mar-97   floh    + nw_class_seg
**                          + windp_class_seg
**      10-Mar-97   floh    + win3d_class_seg
*/
{
    /* Segment-Liste initialisieren */
    nc_NewList((struct List *)&SegmentList);

    /* Debug-Variablen initialisieren */
    NBase.AllocCount = 0;
    NBase.AllocSize  = 0;
    NBase.AllocMax   = 0;
    NBase.FOpenCount = 0;

    /*** hier alle nc_AddSegment()'s ***/
    #ifndef NO_VFM_CLASSES
        nc_AddSegment(&nucleus_class_seg);
        nc_AddSegment(&rsrc_class_seg);
        nc_AddSegment(&bitmap_class_seg);
        nc_AddSegment(&skeleton_class_seg);
        nc_AddSegment(&ilbm_class_seg);
        nc_AddSegment(&sklt_class_seg);
        nc_AddSegment(&ade_class_seg);
        nc_AddSegment(&area_class_seg);
        nc_AddSegment(&base_class_seg);
        nc_AddSegment(&bmpanim_class_seg);
        nc_AddSegment(&amesh_class_seg);
        nc_AddSegment(&prtl_class_seg);
        nc_AddSegment(&ebd_class_seg);
        nc_AddSegment(&idev_class_seg);
        nc_AddSegment(&inp_class_seg);
        nc_AddSegment(&itimer_class_seg);
        nc_AddSegment(&iwimp_class_seg);
        nc_AddSegment(&sample_class_seg);
        nc_AddSegment(&wav_class_seg);
        nc_AddSegment(&rst_class_seg);
        nc_AddSegment(&disp_class_seg);
        nc_AddSegment(&bt_class_seg);
        nc_AddSegment(&rq_class_seg);
        #ifdef __NETWORK__
        nc_AddSegment(&nw_class_seg);
        #endif

        #ifdef __WINDOWS__
            /*** Windows-Only ***/
            nc_AddSegment(&windd_class_seg);
            nc_AddSegment(&win3d_class_seg);
            nc_AddSegment(&winp_class_seg);
            nc_AddSegment(&wit_class_seg);
            #ifdef __NETWORK__
            nc_AddSegment(&windp_class_seg);
            #endif
        #endif
        #ifdef __DOS__
            /*** DOS-Only ***/
            nc_AddSegment(&dmouse_class_seg);
            nc_AddSegment(&dkey_class_seg);
            nc_AddSegment(&dtimer_class_seg);
            nc_AddSegment(&milestimer_class_seg);
            nc_AddSegment(&vga_class_seg);
            #ifdef VFM_3DBLASTER
                /*** DOS- und 3D-Blaster-Only ***/
                nc_AddSegment(&cgl_class_seg);
            #endif
        #endif
    #endif

    #ifndef NO_VFM_ENGINES
        nc_AddSegment(&ove_engine_seg);
        nc_AddSegment(&tform_ng_engine_seg);
        nc_AddSegment(&ie_engine_seg);
        nc_AddSegment(&audio_miles_engine_seg);
    #endif

    #ifndef NO_YPA
        nc_AddSegment(&yw_class_seg);
        nc_AddSegment(&yb_class_seg);
        nc_AddSegment(&yt_class_seg);
        nc_AddSegment(&yr_class_seg);
        nc_AddSegment(&ym_class_seg);
        nc_AddSegment(&yf_class_seg);
        nc_AddSegment(&yc_class_seg);
        nc_AddSegment(&yu_class_seg);
        nc_AddSegment(&yg_class_seg);
    #endif

    /* Ende */
    return(TRUE);
};

/*-----------------------------------------------------------------*/
void nc_SystemCleanup(void)
/*
**  FUNCTION
**      Bis jetzt leer...
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      28-Dec-94   floh    created
**      26-Apr-96   floh    logged __FopenCount und __AllocCount
**      24-Feb-97   floh    Diagnose-Variablen in NBase eingebettet
*/
{
    _LogMsg("Nucleus shutdown:\n");
    _LogMsg("    __AllocCount = %d\n",NBase.AllocCount);
    _LogMsg("    __AllocSize  = %d\n",NBase.AllocSize);
    _LogMsg("    __AllocMax   = %d\n",NBase.AllocMax);
    _LogMsg("    __FOpenCount = %d\n",NBase.FOpenCount);
}

