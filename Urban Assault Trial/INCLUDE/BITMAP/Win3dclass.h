#ifndef BITMAP_WIN3DCLASS_H
#define BITMAP_WIN3DCLASS_H
/*
**  $Source: PRG:VFM/Include/bitmap/win3dclass.h,v $
**  $Revision: 38.2 $
**  $Date: 1998/01/06 12:54:53 $
**  $Locker: floh $
**  $Author: floh $
**
**  Direct3D Display Treiber Klasse im Immediate Mode.
**
**  (C) Copyright 1997 by A.Weissflog
*/
#ifdef WIN3D_WINBOX

    #ifndef BITMAP_WINDDCLASS_H
    #define WINDD_WINBOX
    #include "bitmap/winddclass.h"
    #endif

#else

    /*** PUBLIC ***/
    #ifndef EXEC_TYPES_H
    #include <exec/types.h>
    #endif

    #ifndef BITMAP_WINDDCLASS_H
    #include "bitmap/winddclass.h"
    #endif

#endif

/*-------------------------------------------------------------------
**  NAME
**      drivers/gfx/win3d.class -- Treiberklasse für D3D
*/
#ifndef WIN3D_WINBOX

    #define WIN3DM_BASE    (DISPM_BASE+METHOD_DISTANCE)

    #define WIN3DA_BASE        (DISPA_BASE+ATTRIB_DISTANCE)

    #define WIN3D_CLASSID "drivers/gfx/win3d.class"

    struct win3d_data {
        void *p;            // Pointer auf "wahren" Datenbereich
        ULONG dither;           // TRUE: Dithering on
        ULONG filter;           // TRUE: Textur-Filtering on
        ULONG antialias;        // TRUE: Edge Antialiasing on
        ULONG alpha;            // Base-Alpha-Wert, 0..255
        ULONG zbufwhentracy;
        ULONG colorkey;         // Colorkey-Transparenz verwenden
    };

#else

    #define W3DSTATE_NUM    (11)

    #define W3DSTATE_TEXTUREHANDLE      (0)
    #define W3DSTATE_TEXTUREPERSPECTIVE (1)
    #define W3DSTATE_SHADEMODE          (2)
    #define W3DSTATE_STIPPLEENABLE      (3)
    #define W3DSTATE_SRCBLEND           (4)
    #define W3DSTATE_DESTBLEND          (5)
    #define W3DSTATE_TEXTUREMAPBLEND    (6)
    #define W3DSTATE_BLENDENABLE        (7)
    #define W3DSTATE_ZWRITEENABLE       (8)
    #define W3DSTATE_MAGTEXTUREFILTER   (9)
    #define W3DSTATE_MINTEXTUREFILTER   (10)

    struct w3d_RenderState {
        unsigned long id;
        unsigned long status;
    };

    struct w3d_PixelFormat {
        unsigned long byte_size;    // Größe eines Pixels in Bytes
        unsigned long shift_size;   // X*Y-Shift-Wert für Adress-Berechnung
        unsigned long color_key;    // korrekter ColorKey
        long r_mask,r_shift;
        long g_mask,g_shift;
        long b_mask,b_shift;
        long a_mask,a_shift;
        void *rmap_table;           // Remap-Tabelle
    };

    struct w3d_Execute {
        struct w3d_RenderState glb_state[W3DSTATE_NUM];
        struct w3d_RenderState cur_state[W3DSTATE_NUM];
        unsigned long flags;        // siehe unten
        void *inst_start;
        void *vertex_start;
        void *start;                // Start des Execute Buffers
        void *end;                  // Ende des Execute Buffers
        void *inst;                 // zeigt nach letzter Instruktion
        void *vertex;               // zeigt auf 1.Vertex
        long vertex_index;          // dessen Index zum Anfang des Buffers
        long vertex_count;          // Anzahl Vertices
        void *rs_start;             // Temp Pointer fuer Renderstates
        long rs_count;
        void *tri_start;            // Temp Pointer fuer Triangles
        long tri_count; 
        long begin_scene_ok;        // TRUE, wenn BeginScene geklappt hat
    };
    #define W3D_EBF_ISLOCKED      (1<<0)  // Buffer ist gueltig und kann beschrieben werden
    #define W3D_EBF_NOSTATECHANGE (1<<1)  // Renderstate wurde seit letztem Primitive geandert

    struct w3d_TxtSlot {
        unsigned long flags;        // siehe unten
        unsigned long cache_hits;   // aufeinanderfolgende CacheHits
        LPDIRECTDRAWSURFACE lpSurface;      // Surface der Textur
        LPDIRECT3DTEXTURE   lpTexture;      // Textur-Interface
        LPDIRECT3DTEXTURE2  lpTexture2;
        LPDIRECTDRAWPALETTE lpTxtPalette;   // falls CLUT Textur
        LPDIRECT3DTEXTURE2  lpSource;       // die Source-Textur
        struct w3d_BmpAttach *attach;
    };
    #define W3DF_TSLOT_FLUSHME  (1<<0)  // Slot zum Überschreiben frei
    #define W3DF_TSLOT_USED     (1<<1)  // Slot besetzt

    /*** Win3D-Version von VFMBitmap ***/
    struct w3d_VFMBitmap {
        void *Data;
        unsigned short Width;
        unsigned short Height;
        unsigned long BytesPerRow;
        void *ColorMap;
        unsigned long Flags;
        void *TxtHandle;
        void *TxtCLUT;
    };

    /*** Winbox-Version von RastPoly ***/
    struct w3d_RastPoly {
        unsigned long flags;
        unsigned long pnum;
        float *xyz;             // [x,y,z]
        float *uv;              // [u,v]
        float *b;               // [b]
        void *map[1];
    };
    #define W3DF_RPOLY_LINMAP       (1<<0)
    #define W3DF_RPOLY_PERSPMAP     (1<<1)
    #define W3DF_RPOLY_FLATSHADE    (1<<2)
    #define W3DF_RPOLY_GRADSHADE    (1<<3)
    #define W3DF_RPOLY_ZEROTRACY    (1<<4)
    #define W3DF_RPOLY_LUMTRACY     (1<<5)

    /*** Win3D-Version der Font-Strukturen (DONT CHANGE!) ***/
    struct w3d_FontChar {
        unsigned short offset;  // in Pixel!
        unsigned short width;   // in Pixel!
    };
    struct w3d_VFMFont {
        void *page_master;                  // real: Object *
        struct w3d_VFMBitmap *page_bmp;     // real: VFMBitmap *
        void *page;                         // real: Pointer auf Bitmap-Data-Block
        struct w3d_FontChar *fchars;
        unsigned short height;
    };

    /*** wird an VFMBitmap-Textur attached ***/
    struct w3d_BmpAttach {
        LPDIRECTDRAWSURFACE lpSurface;
        LPDIRECT3DTEXTURE   lpTexture;
        LPDIRECTDRAWPALETTE lpPalette;
    };

    /*** Polygone, die verzögert werden müssen ***/
    struct w3d_DelayedPoly {
        struct w3d_RastPoly *p;           // Pointer auf Polygon-Definition
        struct w3d_BmpAttach *bmp_attach; // Pointer auf Textur-Definition
        float max_z;
    };

    #define W3D_MAXNUM_DELAYED     (512)
    #define W3D_MAXNUM_VERTEX      (12)
    #define W3D_MAXNUM_TXTSLOTS    (32)

    /*** Hardcoded Defs für Paletten-Slots ***/
    #define W3D_PAL_NUMSLOTS    (8)
    #define W3D_PAL_SLOT0   (0x00ffffff)
    #define W3D_PAL_SLOT1   (0x00ff0000)
    #define W3D_PAL_SLOT2   (0x000000ff)
    #define W3D_PAL_SLOT3   (0x0000ff80)
    #define W3D_PAL_SLOT4   (0x00ffffff)
    #define W3D_PAL_SLOT5   (0x00ffffff)
    #define W3D_PAL_SLOT6   (0x00ffffff)
    #define W3D_PAL_SLOT7   (0x0000ffc0)

    #define W3D_COPPER_LINES (255)

    struct w3d_Data {

        /*** X/Y-Skalierer ***/
        float x_scale;      // Skalierer für Display-Koord-Konvertierung
        float y_scale;

        /*** Texture Cache ***/
        unsigned long num_slots;
        struct w3d_TxtSlot slot[W3D_MAXNUM_TXTSLOTS];

        /*** Delayed Polygons ***/
        unsigned long num_solid;
        struct w3d_DelayedPoly solid[W3D_MAXNUM_DELAYED];
        unsigned long num_tracy;
        struct w3d_DelayedPoly tracy[W3D_MAXNUM_DELAYED];

        /*** Display Pixelformat Daten ***/
        struct w3d_PixelFormat disp_pfmt;       // Pixelformat des Displays
        struct w3d_PixelFormat txt_pfmt;        // Pixelformat der Texturen
        unsigned long line_remap_table[W3D_COPPER_LINES];   // für Coppereffekte
        unsigned long line_actindex;            // aktueller Zeilen-Index

        /*** Execute Buffers ***/
        struct w3d_Execute exec;        // Execute Buffer Daten

        /*** Paletten-Effekte ***/
        float pal_slot[W3D_PAL_NUMSLOTS][3];
        float pal_r,pal_g,pal_b;        // 0.0 .. 1.0
    };

    struct win3d_data {
        struct w3d_Data *p;
        unsigned long dither;           // TRUE: Dithering on
        unsigned long filter;           // TRUE: Textur-Filtering on
        unsigned long antialias;        // TRUE: Edge Antialiasing on
        unsigned long alpha;            // Base-Alpha-Wert, 0..255
        unsigned long zbufwhentracy;    // TRUE: ZBuffer bei Tracys an
        unsigned long docolorkey;       // TRUE: Colorkey-Transparenz nehmen
    };

#endif

/*-----------------------------------------------------------------*/
#endif

