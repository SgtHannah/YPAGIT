#ifndef BITMAP_RASTERCLASS_H
#define BITMAP_RASTERCLASS_H
/*
**  $Source: PRG:VFM/Include/bitmap/rasterclass.h,v $
**  $Revision: 38.9 $
**  $Date: 1998/01/06 12:54:32 $
**  $Locker:  $
**  $Author: floh $
**
**  Erweitert die bitmap.class um Methoden, mit denen in die Bitmap
**  gemalt werden kann.
**
**  (C) Copyright 1995 by A.Weissflog
*/
#ifndef BITMAP_BITMAPCLASS_H
#include "bitmap/bitmapclass.h"
#endif

#ifndef VISUALSTUFF_FONT_H
#include "visualstuff/font.h"
#endif

/*-------------------------------------------------------------------
**  NAME
**      raster.class -- Stammklasse für alle Klassen, die in eine
**                      Bitmap zeichnen können
**
**  FUNCTION
**      Die raster.class ermöglicht es, in ein Bitmap-Objekt zu
**      zeichnen und Bereiche zwischen Bitmap-Objekten zu blitten.
**      Sie ist die Grundlage für ein device-unabhängiges
**      Grafik-Treiber-System.
**      Die raster.class enthält plattformunabhängige CPU-only-
**      Routinen, mit denen in 8-Bit-VFMBitmaps gerendert
**      werden kann. Anpassungen an spezielle Hardwares
**      können durch Ableitung von der raster.class erzeugt
**      werden.
**
**      Die raster.class unterstützt zwei 2D-Koordinaten-Systeme:
**      COORD_Int und COORD_Float:
**
**          COORD_Int:    benutzt absolute Integer-Pixel-Koords,
**              x:  [-BMA_Width/2 .. +BMA_Width/2]
**              y:  [-BMA_Width/2 .. +BMA_Width/2]
**
**          COORD_Float:    benutzt auflösungsabhängige Float-Koords,
**              x:  [-1.0 .. +1.0]
**              y:  [-1.0 .. +1.0]
**
**      Für die Render-Methoden, wo das Sinn macht, werden je
**      2 Versionen angeboten, eine mit COORD_Int und eine
**      mit COORD_Float Interface, einige Methoden stehen
**      nur in 1 Version zur Verfügung. Das "Default-Interface"
**      ist COORD_Float, deshalb sind die Namen der FLOAT-Methoden
**      nicht durch ein extra Prefix gekennzeichnet.
**
**  METHODS
**      RASTM_Clear
**          Msg:    ---
**          Ret:    ---
**
**          Löscht die gesamte Bitmap in der Farbe RASTA_BGPen.
**
**      RASTM_Copy
**          Msg:    struct rast_copy_msg
**          Ret:    ---
**
**          Kopiert die gesamte Bitmap in eine VFMBitmap-Struktur.
**          Die Ziel-Bitmap muß in Größe und Format *identisch* sein.
**          Diese Beschränkungen erlauben ein sehr schnelles Kopieren. 
**
**      RASTM_Rect
**      RASTM_IntRect
**          Msg:    struct rast_rect
**                  struct rast_intrect
**          Ret:    ---
**
**          Rendert ein 1-farbiges ungeclipptes Rechteck
**          in die Bitmap in der Farbe RASTA_FGPen.
**
**      RASTM_ClippedRect
**      RASTM_IntClippedRect
**          Msg:    struct rast_rect
**                  struct rast_intrect
**          Ret:    ---
**
**          Rendert ein 1-farbiges geclipptes Rechteck in
**          der Farbe RASTA_FGPen.
**
**      RASTM_Line
**      RASTM_IntLine
**          Msg:    struct rast_line
**                  struct rast_intline
**          Ret:    ---
**
**          Rendert eine 1-farbige ungeclippte Linie in der
**          Farbe RASTA_FGPen.
**
**      RASTM_ClippedLine
**      RASTM_IntClippedLine
**          Msg:    struct rast_line
**                  struct rast_intline
**          Ret:    ---
**
**          Rendert eine 1-farbige geclippte Linie in der Farbe
**          RASTA_FGPen.
**
**      RASTM_Blit
**      RASTM_IntBlit
**      RASTM_MaskBlit
**      RASTM_IntMaskBlit
**          Msg:    struct rast_blit
**                  struct rast_intblit
**                  struct rast_maskblit
**                  struct rast_intmaskblit
**          Ret:    ---
**
**          Kopiert einen rechteckigen Bereich aus einer
**          Source-Bitmap in das raster.class Object.
**          Source und Target dürfen nicht geclippt sein
**          und dürfen unterschiedlich groß sein.
**          (bei identischer Größe wird optimaler
**          geblittet, jedenfalls sollte das theoretisch so
**          sein ;-/).
**          Die Masken-Versionen erwarten eine zusätzliche
**          8-Bit-Bitmap, die dieselbe Ausdehnung wie die Src-
**          Bitmap hat, und einen 8-Bit-Masken-Key.
**
**      RASTM_ClippedBlit
**      RASTM_IntClippedBlit
**          Msg:    struct rast_blit
**                  struct rast_intblit
**          Ret:    ---
**
**          Entspricht RASTM_Blit, Start- und/oder Zielbereich
**          dürfen jedoch voll oder teilweise außerhalb
**          der Bitmap liegen.
**
**      RASTM_ClipRegion
**      RASTM_IntClipRegion
**		RASTM_InvClipRegion
**		RASTM_IntInvClipRegion
**          Msg:    struct rast_rect
**                  struct rast_intrect
**          Ret:    ---
**
**          Definiert das Clip-Rect, welches diverse Routinen
**          zum Clippen benutzen.
**
**      RASTM_Poly
**          Msg:    struct rast_poly
**          Ret:    ---
**
**          Zeichnet einen ungeclippten(!) 2D-Polygon.
**          Bitte beachten, daß es *keine* COORD_Int und keine
**          geclippte Version gibt!!!
**
**      RASTM_SetFont
**          Msg:    struct rast_font
**          Ret:    ---
**
**          Verknüpfe eine VFMFont-Struktur mit einer ID.
**
**      RASTM_GetFont
**          Msg:    struct rast_font
**          Ret:    void (ausgefüllte Msg)
**
**          Falls eine FontID gegeben ist (und der VFMFont-Pointer == NULL), 
**          wird der zugehörige VFMFont-Pointer zurückgeschrieben.
**          Falls der VFMFont-Pointer gegeben ist, wird das
**          interne Font-Array nach diesem Pointer durchsucht,
**          und bei Erfolgsfall die gefundene FontID zurückgeschrieben,
**          andernfalls -1.
**
**      RASTM_Text
**          Msg:    struct rast_text
**          Ret:    ---
**
**          Rendert den übergebenen Text-String (mit eingebetteten
**          Kontroll-Sequenzen) in die VFMBitmap. Bitte beachten,
**          daß der Text generell durch die TracyLUM gefiltert wird.
**
**      RASTM_SetFog
**          Msg:    struct rast_fog
**          Ret:    ---
**
**          Setzt die Fogging-Parameter FogStart, FogEnd und
**          FogColor.
**
**      RASTM_Begin3D
**      RASTM_End3D
**          Msg:    ---
**          Ret:    ---
**
**          Alle Polygon-Rendering-Operationen müssen in ein
**          RASTM_Begin3D/RASTM_End3D-Paar eingeschlossen sein.
**          Es darf nur 1 RASTM_Begin3D/RASTM_End3D-Paar pro
**          Frame vorhanden sein.
**          Diese Methoden ersetzen NICHT die übergeordneten
**          DISPM_Begin/DISPM_End!!!!!!!!!!!!!!!!!!!!!!!!!!!!
**
**
**      RASTM_Begin2D
**      RASTM_End2D
**          Msg:    ---
**          Ret:    ---
**
**          Alle 2D-Operationen (Text/Lines/Blits) müssen
**          in ein RASTM_Begin2D/RASTM_End2D Paar eingeschlossen
**          sein, es dürfen mehrere RASTM_Begin2D/RASTM_End2D
**          pro Frame vorhanden sein.
**
**      RASTM_SetPens
**          Msg:    struct rast_pens
**          Ret:    ---
**
**          Setzt einen oder mehrere der Pens gleichzeitig.
**
**  ATTRIBUTES
**
**      RASTA_FGPen  (ISG) - <ULONG>
**      RASTA_FGAPen (ISG) - <ULONG>
**          Definiert den momentan gültigen Foreground-Pen.
**          Pens werden in der raster.class definiert als
**          CLUT-Index, allerdings können (zukünftige)
**          Truecolor-Subklassen diese Definition aufheben.
**
**      RASTA_BGPen (ISG) - <ULONG>
**          Definiert den momentan gültigen Background-Pen.
**
**      RASTA_ShadeLUM  (ISG) - <struct VFMBitmap *>
**          Definiert eine VFMBitmap, welche als Lookup-Table
**          fürs Shading verwendet werden soll. Die Bitmap-Größe
**          muß 256x256 sein.
**
**      RASTA_TracyLUM  (ISG) - <struct VFMBitmap *>
**          Wie RASTA_ShadeLUM, nur fürs Transparenz-Lookup.
**
**-----------------------------------------------------------------*/
#define RASTM_BASE      (BMM_BASE+METHOD_DISTANCE)

#define RASTM_Clear             (RASTM_BASE)
#define RASTM_Copy              (RASTM_BASE+1)
#define RASTM_Rect              (RASTM_BASE+2)
#define RASTM_IntRect           (RASTM_BASE+3)
#define RASTM_ClippedRect       (RASTM_BASE+4)
#define RASTM_IntClippedRect    (RASTM_BASE+5)
#define RASTM_Line              (RASTM_BASE+6)
#define RASTM_IntLine           (RASTM_BASE+7)
#define RASTM_ClippedLine       (RASTM_BASE+8)
#define RASTM_IntClippedLine    (RASTM_BASE+9)
#define RASTM_Blit              (RASTM_BASE+10)
#define RASTM_IntBlit           (RASTM_BASE+11)
#define RASTM_ClippedBlit       (RASTM_BASE+12)
#define RASTM_IntClippedBlit    (RASTM_BASE+13)
#define RASTM_Poly              (RASTM_BASE+14)
#define RASTM_SetFont           (RASTM_BASE+15)
#define RASTM_GetFont           (RASTM_BASE+16)
#define RASTM_Text              (RASTM_BASE+17)
#define RASTM_ClipRegion        (RASTM_BASE+18)
#define RASTM_IntClipRegion     (RASTM_BASE+19)
#define RASTM_SetFog            (RASTM_BASE+20)
#define RASTM_Begin3D           (RASTM_BASE+21)
#define RASTM_End3D             (RASTM_BASE+22)
#define RASTM_Begin2D           (RASTM_BASE+23)
#define RASTM_End2D             (RASTM_BASE+24)
#define RASTM_SetPens           (RASTM_BASE+25)
#define RASTM_MaskBlit          (RASTM_BASE+26)
#define RASTM_IntMaskBlit       (RASTM_BASE+27)
#define RASTM_InvClipRegion		(RASTM_BASE+28)
#define RASTM_IntInvClipRegion	(RASTM_BASE+29)

/*-----------------------------------------------------------------*/
#define RASTA_BASE      (BMA_BASE+ATTRIB_DISTANCE)

#define RASTA_FGPen     (RASTA_BASE)      /* (ISG) */
#define RASTA_BGPen     (RASTA_BASE+1)    /* (ISG) */
#define RASTA_ShadeLUM  (RASTA_BASE+2)    /* (ISG) */
#define RASTA_TracyLUM  (RASTA_BASE+3)    /* (ISG) */
#define RASTA_FGAPen    (RASTA_BASE+4)    /* (ISG) */

/*-----------------------------------------------------------------*/
#define RASTER_CLASSID "raster.class"

#ifdef AMIGA
#define DRAWSPAN_PREFIX __asm void
#define DRAWSPAN_ARGS   __a0 UBYTE *, __a1 struct rast_scanline *, __a2 struct raster_data *
#else
#ifdef _MSC_VER
#define DRAWSPAN_PREFIX void
#define DRAWSPAN_CALL __fastcall
#else
#define DRAWSPAN_PREFIX void
#define DRAWSPAN_CALL
#endif
#define DRAWSPAN_ARGS   UBYTE *, struct rast_scanline *, struct raster_data *
#endif

/*-----------------------------------------------------------------*/
struct rast_fog {
    ULONG enable;
    FLOAT start;
    FLOAT end;
    FLOAT r,g,b;
};

struct rast_copy_msg {
    struct VFMBitmap *to;
};

struct rast_rect {
    FLOAT xmin,ymin;
    FLOAT xmax,ymax;
};

struct rast_intrect {
    LONG xmin,ymin;
    LONG xmax,ymax;
	ULONG flags;
};

struct rast_line {
    FLOAT x0,y0;
    FLOAT x1,y1;
};

struct rast_intline {
    LONG x0,y0;
    LONG x1,y1;
};

struct rast_blit {
    struct VFMBitmap *src;
    struct rast_rect from;  // Source-Rechteck
    struct rast_rect to;    // Target-Rechteck
};

struct rast_intblit {
    struct VFMBitmap *src;
    struct rast_intrect from;
    struct rast_intrect to;
};

struct rast_maskblit {
    struct VFMBitmap *src;
    struct VFMBitmap *mask;
    ULONG mask_key;
    struct rast_rect from;  // Source-Rechteck
    struct rast_rect to;    // Target-Rechteck
};

struct rast_intmaskblit {
    struct VFMBitmap *src;
    struct VFMBitmap *mask;
    ULONG mask_key;
    struct rast_intrect from;
    struct rast_intrect to;
};

/*** private: Eckpunkt eines Polys ***/
struct rast_ppoint {
    LONG  x,y;
    ULONG z,u,v,b;
};

/*** private: Scanline-Definition eines Polys ***/
struct rast_scanline {
    DRAWSPAN_PREFIX (DRAWSPAN_CALL *draw_span) (DRAWSPAN_ARGS);
    WORD flags;         // Poly-Flags
    WORD y;
    WORD x0,dx;
    ULONG b0,z0,u0,v0;
    LONG  db,dz,du,dv;
    UBYTE *map;         // Pointer in Texture-Map
};

/*** private: Definition eines Map-Clusters ***/
struct rast_cluster {
    LONG count;     // Pixel im Cluster
    LONG m0;        // [u0|v0]
    LONG dm;        // [du|dv]
};

struct rast_xyz {
    FLOAT x,y,z;        // x,y: [0.0 .. 1.0] z: [16.0 .. n]
};

struct rast_uv {
    FLOAT u,v;          // [0.0 .. 1.0]
};

struct rast_font {
    struct VFMFont *font;
    LONG id;
};

struct rast_text {
    UBYTE *string;      // der String mit Kontroll-Sequenzen
    UBYTE **clips;      // maybe NULL, wenn <string> keine <clips> enthält
};

struct rast_pens {
    LONG fg_pen;        // -1 -> nicht setzen
    LONG fg_apen;
    LONG bg_pen;
};

/*** für Spangine-Modul ***/
struct rast_minspan {
    struct MinNode nd;
    WORD x0,dx;
};

/*-------------------------------------------------------------------
**  Definition eines Polygons:
**  ~~~~~~~~~~~~~~~~~~~~~~~~~~
**  Die Sache ist nicht ganz trivial. Neben den Display-Koords
**  müssen ja auch fast beliebig weitere Pro-Vertex-Attribute angegeben
**  werden können. Also:
**
**  rast_poly.flags
**  ---------------
**  Hiermit wird der zu verwendende Polygon-Rasterizer
**  definiert. Flags, die in Gruppen angeordnet sind,
**  schließen sich gegenseitig aus:
**
**      RPF_LinMap        - Poly wird [u,v] linear gemapped
**      RPF_PerspMap      - Poly wird [u,v] perspektiv gemapped
**
**      RPF_FlatShade     - Poly ist flat-geshadet
**      RPF_GradShade     - Poly ist Gradient-shaded
**
**      RPF_ZeroTracy     - Pen 0 ist durchsichtig
**      RPF_LUMTracy      - Transparenz per RASTA_TracyLUM
**
**  Folgende Kombinationen werden derzeit unterstützt:
**
**      ---                             benutzt [xyz]
**      (RPF_FlatShade)                 benutzt [xyz][b]
**      (RPF_GradShade)                 benutzt [xyz][b]
**      (RPF_LinMap)                    benutzt [xyz][uv]
**      (RPF_PerspMap)                  benutzt [xyz][uv]
**
**      (RPF_LinMap   | RPF_FlatShade)  benutzt [xyz][b][uv]
**      (RPF_LinMap   | RPF_GradShade)  benutzt [xyz][b][uv]
**      (RPF_PerspMap | RPF_FlatShade)  benutzt [xyz][b][uv]
**      (RPF_PerspMap | RPF_GradShade)  benutzt [xyz][b][uv]
**
**      (RPF_LinMap   | RPF_ZeroTracy)  benutzt [xyz][uv]
**      (RPF_LinMap   | RPF_LUMTracy)   benutzt [xyz][uv]
**      (RPF_PerspMap | RPF_ZeroTracy)  benutzt [xyz][uv]
**      (RPF_PerspMap | RPF_LUMTracy)   benutzt [xyz][uv]
**
**  Nicht unterstützte Kombinationen werden mit einer
**  Fehlermeldung ignoriert.
**
**  rast_poly.pnum
**  --------------
**  Definiert Anzahl der Elemente im Input-Stream, das
**  entspricht natürlich einfach der Anzahl der Ecken des 
**  Polygons.
**
**  rast_poly.p
**  -----------
**  Zeigt auf einen FLOAT-Stream von Per-Vertex-Attributen.
**  Für den Aufbau des Streams bitte die og. Polyflag-Kombinationen
**  beachten.
**
**  rast_poly.map[1]
**  ----------------
**  Hier wird ein Pointer auf eine VFMBitmap-Struktur für
**  die Texture-Map definiert, falls eines der Mapping-Flags
**  gesetzt ist. rast_poly.map[] ist ein Array, weil es in Zukunft
**  auch Polygone mit mehreren Source-Maps geben kann
**  (Stichwort Overlay-Mapping).
*/

struct rast_poly {
    ULONG flags;
    ULONG pnum;
    struct rast_xyz *xyz;   // [x,y,z] Channel
    struct rast_uv *uv;     // [u,v] Channel
    FLOAT *b;               // [b] Channel
    struct VFMBitmap *map[1];
};

#define RPF_LinMap      (1<<0)
#define RPF_PerspMap    (1<<1)
#define RPF_FlatShade   (1<<2)
#define RPF_GradShade   (1<<3)
#define RPF_ZeroTracy   (1<<4)
#define RPF_LUMTracy    (1<<5)

/*-----------------------------------------------------------------*/
struct raster_data {
    struct VFMBitmap *r;        // eigene VFMBitmap als Output-Raster

    ULONG fg_pen;               // aktueller Foreground-Pen
    ULONG fg_apen;              // aktueller sekundärer Foreground-Pen
    ULONG bg_pen;               // aktueller Background-Pen

    struct VFMBitmap *shade_bmp;
    UBYTE *shade_body;          // shade_bmp->Data

    struct VFMBitmap *tracy_bmp;
    UBYTE *tracy_body;          // tracy_bmp->Data

    struct rast_cluster *cluster_stack;
    struct rast_intrect clip;
	struct rast_intrect inv_clip;

    DRAWSPAN_PREFIX (DRAWSPAN_CALL *drawspan_lut[64])(DRAWSPAN_ARGS); // 64 Spandrawer
    struct VFMFont *fonts[256];

    LONG  ioff_x,ioff_y;
    FLOAT foff_x,foff_y;

    /*** Edge-Tables für Polygon-Renderer ***/
    struct rast_ppoint *left_edge;
    struct rast_ppoint *right_edge;

    /*** Span-Clipper-Variablen ***/
    struct MinList *header_pool;
    struct MinList free_list;
    struct rast_minspan *span_pool;

    struct rast_scanline *glass_stack;
    struct rast_scanline *glass_stack_ptr;
    struct rast_scanline *end_of_glass_stack;
    ULONG spans_used;

    /*** Fog ***/
    struct rast_fog fog;
};

/*** Prototypen der Spangine ***/
BOOL rst_seInitSpangine(struct raster_data *, ULONG, ULONG);
void rst_seKillSpangine(struct raster_data *);
void rst_seNewFrame(struct raster_data *, ULONG);
void rst_seAddSpan(UBYTE *, struct rast_scanline *, struct raster_data *);
void rst_seFlushSpangine(struct raster_data *);
/*-----------------------------------------------------------------*/
#endif
