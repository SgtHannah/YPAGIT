#ifndef BITMAP_CGLCLASS_H
#define BITMAP_CGLCLASS_H
/*
**  $Source: PRG:VFM/Include/bitmap/cglclass.h,v $
**  $Revision: 38.2 $
**  $Date: 1998/01/06 12:53:57 $
**  $Locker:  $
**  $Author: floh $
**
**  Display-Treiber-Klasse für die "Creative Graphics Library (TM)".
**  Sprich: 3D-Blaster-Support.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#define CGLFLOAT (1)
#define HOTCGL (1)
#define VERITE (1)

#ifndef __CGL__
#include "thirdparty/cgl.h"
#endif

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_LISTS_H
#include <exec/lists.h>
#endif

#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif

#ifndef BITMAP_DISPLAYCLASS_H
#include "bitmap/displayclass.h"
#endif

#ifndef PROTOCOL_MSDOS_H
#include "protocol/msdos.h"
#endif

/*-------------------------------------------------------------------
**  NAME
**      drivers/gfx/cgl.class -- Treiberklasse für Creative
**                               Graphics Library.
**
**  FUNCTION
**      Benutzt die CGL als 3D-Api unter DOS. Erstes
**      experimentelles 3D-Accelerator-Interface!
**
**      Der Texture-Cache-Manager geht nach folgendem
**      Masterplan vor:
**
**          - Texturen im Cache die einen Frame lang nicht
**            benutzt wurden, werden als FlushMe gekennzeichnet,
**            das heißt, sie werden zum Überschreiben bei
**            einem CacheMiss freigegeben.
**            (es ist relativ wahrscheinlich, daß eine Textur,
**            die mindestens einen Frame *nicht* benutzt wurde, auch
**            in den unmittelbar folgenden Frames *nicht*
**            benötigt wird).
**
**          - Solange eine Textur hintereinanderfolgend
**            im Cache einen Treffer hat, wird ein
**            CacheHit-Counter inkrementiert, je höher dieser
**            Hit-Count steht, desto mehr "klebt" die
**            Textur im Cache (es ist relativ wahrscheinlich,
**            daß eine Textur, die in den vorhergehenden
**            Frames benutzt wurde, auch in den unmittelbar
**            folgenden Frames benötigt wird!).
**
**          - Tritt ein CacheMiss bei voll besetztem
**            Cache auf (keine Textur hat das FlushMe Bit
**            gesetzt), wird die Textur im Cache mit
**            dem niedrigsten Hit-Count "zwangs-geflusht".
**
**          - Tritt ein CacheMiss bei mehreren
**            FlushMe-Texturen im Cache auf, wird diejenige
**            Textur mit dem niedrigsten Hit-Count
**            geflusht (der HitCount bleibt auch im
**            FlushMe-Zustand solange unverändert, bis
**            die Textur endgültig aus dem Cache
**            geworfen wird, erst dann "verfällt"
**            der HitCount.
**
**  cgl.class Display-IDs
**  ~~~~~~~~~~~~~~~~~~~~~
**  Die Display-ID baut sich folgendermaßen auf:
**
**      ID = ((ScreenMode)<<8) | (ColorFormat)
**
*/
#define CGLM_BASE       (DISPM_BASE+METHOD_DISTANCE)
#define CGLA_BASE       (DISPM_BASE+METHOD_DISTANCE)

#define CGL_CLASSID "drivers/gfx/cgl.class"

/*-----------------------------------------------------------------*/

/*** Texture-Cache-Managment ***/

//
// ein Texture-Slot ist jeweils eine 256x256 Textur im
// ColorModel RGB5551
//
struct TextureSlot {
    ULONG flags;            // siehe unten
    ULONG cache_hits;       // aufeinanderfolgende CacheHits (in Polys)
    ULONG offset;           // WORD(!)-Offset der Textur-Daten im Txt-Buffer
    ULONG txt_handle;       // interne CGL-Adresse der Textur
    struct VFMBitmap *bmp;  // Source-Bitmap, NULL->Slot ist frei
};

#define CGLF_FLUSHME    (1<<0)  // Slot zum überschreiben freigegeben
#define CGLF_USED       (1<<1)  // 1, wenn in diesem Frame benutzt worden

/*** für Polygone, die aus irgendeinem Grund verzögert werden müssen ***/
struct DelayedPoly {
    struct MinNode nd;
    struct rast_poly *p;
};

#define CGL_MAXNUM_DELAYED  (512)   // max. Anzahl Delayed Polygons
#define CGL_MAXNUM_VERTEX   (12)    // max. Anzahl Kanten pro Primitive

/*** eine CGL-Polygon-Struktur ***/
struct cgl_Polygon {
    CGL_RENDERFIX_ST  rend;
    CGL_DEPTH_ST      depth;
    CGL_ALPHABLEND_ST blend;
    CGL_TEXTUREENV_ST text;
};

#define CGL_POLYBUF_MAX    (256)   // max. 256 Polygon im Puffer
#define CGL_VERTEXBUF_MAX  (1024)  // max. 1024 Vertixes im Puffer

struct cgl_data {

    /*** allgemein ***/
    ULONG id;           // die eingestellte Display-Id
    ULONG screen_mode;
    ULONG color_model;
    ULONG screen_ok;    // TRUE, wenn Screen initialisiert wurde
    ULONG flags;        // siehe unten

    FLOAT x_scale;      // Skalierer für Display-Koord-Konvertierung
    FLOAT y_scale;

    /*** Texture-Cache ***/
    UWORD rgb5551_table[256];       // Remap-Tabelle im RGB5551-Format
    ULONG num_slots;                // Anzahl Textur-Slots
    struct TextureSlot *slot;       // Array[num_txtslots]

    /*** Delayed Polygon Listen ***/
    struct DelayedPoly dp_buf[CGL_MAXNUM_DELAYED];
    struct MinList free_list;
    struct MinList delayed_list;
    struct MinList tracy_list;

    /*** Mousepointer ***/
    struct VFMBitmap *ptr;          // Pointer auf Original-Pointer
    LONG ptr_x,ptr_y;               // Pointer-Position
    UBYTE ptr_data[256];            // 32x32x2 bit Cursor-Image

    /*** Clipping ***/
    struct rast_rect clip;          // [-1.0..+1.0][-1.0..+1.0]

    /*** Export ***/
    struct msdos_DispEnv export;

    /*** CGL-Render-Buffer ***/
    ULONG poly_index;               // für <poly_buf>
    ULONG vx_index;                 // für <vertex_buf>,<uv_buf>,...
    ULONG vx;                       // ausgefüllt von cgl_GetPoly()
    CGL_PTR stream[CGL_POLYBUF_MAX];
    struct cgl_Polygon *poly_buf;
    CGL_VERTEX3D_ST    *vertex_buf;
    CGL_TEXTURE2D_ST   *uv_buf;
    CGL_TEXTURE3D_ST   *uvz_buf;
    CGL_COLOR_ST       *color_buf;

    CGL_RENDERFIX_ST  line;         // für Line-Primitives
    CGL_VERTEX2D_ST   line_xy[2];
    CGL_COLOR_ST      line_color[2];
};

#define CGLF_Dither     (1<<0)      // Dithering on/off
#define CGLF_Filter     (1<<1)      // Textur-Filtering on/off

/*** Config-Items ***/
#define CGLCONF_Dither  "gfx.cgl.dither"    // BOOL
#define CGLCONF_Filter  "gfx.cgl.filter"    // BOOL

/*-----------------------------------------------------------------*/
#endif

