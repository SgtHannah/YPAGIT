#ifndef BITMAP_WINDDCLASS_H
#define BITMAP_WINDDCLASS_H
/*
**  $Source: PRG:VFM/Include/bitmap/winddclass.h,v $
**  $Revision: 38.4 $
**  $Date: 1998/01/06 12:55:02 $
**  $Locker: floh $
**  $Author: floh $
**
**  Display-Treiber-Klasse für DirectDraw in normalen
**  Windows-Fenster (vorerst kein Fullscreen-Support,
**  mal sehen...).
**  Das ist NICHT der glorreiche Direct3D-Treiber
**  sondern erstmal was weniger anspruchsvolles.
**
**  Weil es mit dem Win32s SDK Typ-Kollisionen
**  mit den VFM-Datentypen stattfinden, müssen
**  alle Windows-Sachen in eigenen Quelltexten
**  abgeschirmt sein -> dafür definiere man das
**  Symbol WINDD_WINBOX.
**  Sobald dieses Symbol definiert ist, erhält man
**  quasi die "private" Version der Klassendefinition,
**  andernseits die "public" Version.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#ifdef WINDD_WINBOX

    /*** Windows Zeuch (oh nein, ich habe das 'W' Wort benutzt) ***/
    #ifndef _INC_WINDOWS
    #include <windows.h>
    #endif

    #ifndef _INC_WINDOWSX
    #include <windowsx.h>
    #endif

    /*** DDraw Zeuch ***/
    #ifndef __DDRAW_INCLUDED_
    #include <ddraw.h>
    #endif           

    /*** Direct3D Zeuch ***/
    #ifndef _D3D_H_
    #include <d3d.h>
    #endif

    #ifndef MISC_DBCS_H
    #define DBCS_PADDEDCELL
    #include "misc/dbcs.h"
    #endif

#else
    /*** PUBLIC ***/
    #ifndef EXEC_TYPES_H
    #include <exec/types.h>
    #endif

    #ifndef BITMAP_DISPLAYCLASS_H
    #include "bitmap/displayclass.h"
    #endif

#endif

/*-------------------------------------------------------------------
**  DEBUGGING MACROS
*/

//#define WDD_DEBUG   (1)
extern void wdd_Log(char *,...);
#ifdef WDD_DEBUG
    #define ENTERED(x) wdd_Log("-> %s entered\n",x);
    #define LEFT(x)    wdd_Log("-> %s left\n",x);
#else
    #define ENTERED(x) 
    #define LEFT(x)
#endif

/*-------------------------------------------------------------------
**  NAME
**      drivers/gfx/windd.class -- Treiberklasse für DDraw Windowed
**
**  FUNCTION
**      Das Rendering findet in einen Backbuffer statt, der
**      eine von mehreren angebotenen Größen haben kann.
**      Dessen Inhalt wird in ein Windows-Window geblittet,
**      welches resizeable ist.
**      Über das Attrs DISPA_DisplayHandle wird das
**      Window-Handle öffentlich gemacht.
**
**  METHODS
**      WINDDM_EnableGDI
**          Msg:    ---
**          Ret:    ---
**
**          Unternimmt alles, damit GDI in die Primary Surface
**          rendern kann.
**
**      WINDDM_DisableGDI
**          Msg:    ---
**          Ret:    ---
**
**          Macht WINDD_EnableGDI wieder rückgängig.
**
**      WINDDM_GetText
**          Msg:    struct windd_gettext
**          Ret:    ---
**
**          Texteingabe-Routine. Erzeugt ein Windows-Edit-
**          Control und kehrt mit einem gültigem Text,
**          oder NULL zurück. Während der Texteingabe
**          blockiert die Routine. Die Routine macht intern
**          ein EnableGDI und DisableGDI um die Texteingabe
**          herum.
**
**      WINDDM_PlayMovie
**          Msg:    struct windd_playmovie
**          Ret:    ---
**
**          Spielt einen per Filenamen definierten Media-
**          File im Mainwindow des windd.class Objekts
**          ab. Benutzt dazu DirectShow.
**
**      WINDDM_QueryDevice
**          Msg:    struct windd_device
**          Ret:    ---
**
**          Dient zur Abfrage aller unterstützten Rendering-
**          Devices. Zum Starten einer Abfrage muß das
**          <name> Feld auf NULL gesetzt werden, mit jedem
**          Aufruf wird WINDDM_GetDevice dann das jeweils
**          nächste zurückliefern.
**          Die zurückgelieferten Pointer <name> und <guid>
**          sind nur solange gültig, wie das aktuelle
**          Display-Object existiert.
**
**          struct windd_device wdev;
**          wdev.name  = NULL;
**          wdev.guid  = NULL;
**          wdev.flags = 0;
**          do (_methoda(gfxo,WINDDM_GetDevice,&wdev)) {
**              if (wdev.name) {
**                  // <name> und <guid> Pointer irgendwohinkopieren
**                  // (wdev.flags & WINDDF_IsCurrentDevice) ist
**                  // TRUE, falls es sich um das momentan ausgewählte
**                  // Device handelt
**              };
**          } while (wdev.name);
**
**      WINDDM_SetDevice
**          Msg:    struct windd_device
**          Ret:    ---
**
**          Setzt das vom Display-Treiber zu verwendende Device.
**          Die <name> und <guid> Pointer müssen aus einem
**          Aufruf von WINDDM_GetDevice stammen.
**          ACHTUNG: Die Methode schreibt die benötigten Daten
**          nur in den guid3d.def, von wo sie beim Erzeugen des
**          Display-Treiber-Objekt gelesen werden. Damit die
**          Änderung wirklich passiert, muß das Display-Treiber-
**          Object neu erzeugt werden, indem man den aktuellen
**          Display-Mode erneut setzt. Nicht sehr elegant, aber
**          sauber.
**
**  ATTRIBUTES
**      WINDDA_CursorMode   (S)
**          Schaltet zwischen Hard- und Software-Cursor um,
**          (WINDD_CURSORMODE_HW, WINDD_CURSORMODE_SOFT).
**
**      WINDDA_TextureFilter    (S)
**      WINDDA_16BitTextures    (S)
**      WINDDA_UseDrawPrimitive (S)
**      WINDDA_DisableLowres    (S)
**-----------------------------------------------------------------*/

#define WINDD_PAINTMODE_FLIP    (0)
#define WINDD_PAINTMODE_BLACK   (1)

#define WINDD_CURSORMODE_HW     (0)
#define WINDD_CURSORMODE_SOFT   (1)

#ifndef WINDD_WINBOX

    /*** PUBLIC ***/
    #define WINDDM_BASE         (DISPM_BASE+METHOD_DISTANCE)
    #define WINDDM_EnableGDI    (WINDDM_BASE)
    #define WINDDM_DisableGDI   (WINDDM_BASE+1)
    #define WINDDM_GetText      (WINDDM_BASE+2)
    #define WINDDM_PlayMovie    (WINDDM_BASE+3)
    #define WINDDM_QueryDevice  (WINDDM_BASE+4)
    #define WINDDM_SetDevice    (WINDDM_BASE+5)

    #define WINDDA_BASE             (DISPA_BASE+ATTRIB_DISTANCE)
    #define WINDDA_CursorMode       (WINDDA_BASE)
    #define WINDDA_TextureFilter    (WINDDA_BASE+1)
    #define WINDDA_DisableLowres    (WINDDA_BASE+2)
    #define WINDDA_16BitTextures    (WINDDA_BASE+3)
    #define WINDDA_UseDrawPrimitive (WINDDA_BASE+4)

    #define WINDD_CLASSID "drivers/gfx/windd.class"

    struct win_DispEnv {
        ULONG hwnd;             // Window-Handle
        ULONG x_size;           // Größe des Background-Framebuffer
        ULONG y_size;
    };

    struct windd_data {
        ULONG hWnd;
        ULONG hInstance;
        ULONG lpDDSPrim;
        ULONG lpDDSBack;        // Flip oder Sysmem
        ULONG lpDDSBackup;
        ULONG lpDDClipper;
        ULONG lpDDPal;
        ULONG back_w,back_h;
        UBYTE *back_ptr;
        ULONG cur_ptr_type;
        ULONG back_pitch;
        ULONG forcesoftcursor;
        ULONG movieplayer;
        ULONG paintmode;
        ULONG forcealphatextures;
        ULONG usedrawprimitive;
        ULONG disablelowres;
        ULONG exportwindowmode;

        ULONG id;
        ULONG flags;
        struct VFMBitmap *r;
        void *d3d;              // Direct3D-Daten...
    };

    struct windd_gettext {
        char *title_text;       // In: Text für Window-Title
        char *ok_text;          // In: Text für OK Button
        char *cancel_text;      // In: Text für Cancel Button
        char *default_text;     // In: optionale Vorgabe für Stringgadget
        ULONG timer_val;        // In: Callback Time in Millisec
        void (*timer_func)(void *);   // In: Timer-Callback-Routine
        void *timer_arg;        // In: optionales Arg für die TimerFunc
        ULONG flags;            // In: siehe unten
        ULONG max_text_len;     // In: maximale Textlaenge in Characters!
        char *result;           // Out: Ergebnis, darf NULL sein
    };
    #define WINDDF_GETTEXT_ISFILENAME   (1<<0)      // entfernt alle ungueltigen Zeichen aus dem String

    struct windd_playmovie {
        char *fname;            // In: Filename des zu spielenden Mediafiles
    };

    struct windd_paintmode {
        ULONG pmode;
    };

    struct windd_device {
        UBYTE *name;
        void *guid;
        ULONG flags;
    };

    struct windd_devnode {
        struct MinNode nd;
        UBYTE name[256];
        UBYTE guid[256];
        ULONG flags;
    };

    #define WINDDF_IsCurrentDevice  (1<<0)

#else
    /*** gespiegelte Gummizellen-Strukturen DONT CHANGE! ***/
    struct wdd_VFMBitmap {
        void *Data;
        unsigned short Width;
        unsigned short Height;
        unsigned long BytesPerRow;
        void *ColorMap;
        unsigned long Flags;
        void *TxtHandle;
        void *TxtCLUT;
    };
    struct wdd_FontChar {
        unsigned short offset;  // in Pixel!
        unsigned short width;   // in Pixel!
    };
    struct wdd_VFMFont {
        void *page_master;                  // real: Object *
        struct wdd_VFMBitmap *page_bmp;     // real: VFMBitmap *
        void *page;                         // real: Pointer auf Bitmap-Data-Block
        struct wdd_FontChar *fchars;
        unsigned short height;
    };

    /*** allgemeine Konstanten ***/
    #define WINDD_MAXNUMMODES      (64)
    #define WINDD_MAXNUMDRIVERS    (16)
    #define WINDD_MAXNUMTXTFORMATS (32)

    /*** 3D: Beschreibung eines 3D-Drivers ***/
    struct wdd_3DDriver {
        D3DDEVICEDESC Desc; // Komplette Beschreibung
        GUID DDrawGuid;                 // GUID des DDraw Objects
        GUID DevGuid;                   // GUID des Treibers
        unsigned short DDrawGuidIsValid;
        unsigned short DevGuidIsValid;
        char DDrawName[256];
        char DDrawDesc[256];
        char DeviceName[256];
        char DeviceDesc[256];
        unsigned long IsHardware;       // es ist ein Hardware-Device
        unsigned long CanDoWindow;      // kann in akt. Primary-Tiefe rendern
        unsigned long DoesZBuffer;      // arbeitet mit Z-Buffer
        unsigned long CanDoAlpha;       // DestBlend & SrcAlpha supported
        unsigned long CanDoStipple;     // Alpha-Emulation durch Stippling
        unsigned long DoSoftCursor;     // benutze emulierten Cursor
        unsigned long CanDoSysMemTxt;   // akzeptiert Texturen im SysMem
        unsigned long CanDoColorKeyTxt; // kann Colorkey-Transparenz
        unsigned long CanDoAdditiveBlend; // kann additives Blending
        unsigned long ZBufBitDepth;
    };

    struct wdd_TxtFormat {
        DDSURFACEDESC ddsd;         // komplette Surface-Beschreibung
        unsigned long IsPalettized; // eine CLUT Texture?
        unsigned long RedBPP;
        unsigned long GreenBPP;
        unsigned long BlueBPP;
        unsigned long AlphaBPP;
        unsigned long RGBBPP;
        unsigned long IndexBPP;     // Anzahl Bits im Paletten-Index
    };

    /*** interne Beschreibung eines DisplayModes ***/
    struct wdd_DisplayMode {
        unsigned long w;
        unsigned long h;
        unsigned long bpp;
        unsigned long can_do_3d;    // kann aktueller Treiber in diesem Mode rendern?
    };

    /*** globale Database ***/
    struct wdd_Data {
        /*** allgemein ***/
        unsigned long Flags;        // siehe unten

        /*** Display-Modes ***/
        unsigned long NumModes;
        struct wdd_DisplayMode Modes[WINDD_MAXNUMMODES];

        /*** Abdruck der Desktop-Surface und der Primary Surface ***/
        DDSURFACEDESC Desktop;
        DDSURFACEDESC Primary;
        unsigned long VidMemTotal;

        /*** das benutzte 3D-Device (wird bevorzugt ein HW-Device sein) ***/
        struct wdd_3DDriver Driver;
        unsigned long ActDriver;            // der hier ist ok
        unsigned long NumDrivers;
        struct wdd_3DDriver AllDrivers[32]; // alle gefundenen Treiber

        /*** Datenstruktur des DBCS-Moduls ***/
        struct dbcs_Handle *DBCSHandle;
    };

    /*** Definitionen für wdd_Data.Flags ***/
    #define WINDDF_CanDoWindowed    (1<<0)  // Driver supp Accel für Primary Surface

    /*** Sammelstruktur für Direct3D-Environment ***/
    struct wdd_D3D {
        LPDIRECTDRAWSURFACE     lpDDSZBuffer;           // nur falls unterstützt...
        LPDIRECT3DDEVICE        lpD3DDevice;
        LPDIRECT3DVIEWPORT2     lpD3DViewport;
        LPDIRECT3DEXECUTEBUFFER lpD3DExecBuffer;
        LPDIRECT3DMATERIAL2     lpBackMat;              // Background-Material
        LPDIRECT3DDEVICE2       lpD3DDevice2;

        /*** Textur-Formate, die der Treiber unterstützt ***/
        unsigned long NumTxtFormats;
        unsigned long ActTxtFormat;
        struct wdd_TxtFormat TxtFormats[WINDD_MAXNUMTXTFORMATS];
    };

    /*** LID auf der Windows-Seite ***/
    struct windd_data {

        HWND hWnd;                          // Window-Handle
        HINSTANCE hInstance;                // App-Instance-Handle

        LPDIRECTDRAWSURFACE lpDDSPrim;      // Primary Surface
        LPDIRECTDRAWSURFACE lpDDSBack;      // Backbuffer
        LPDIRECTDRAWSURFACE lpDDSBackup;    // Backup-Surface für EnableGDI()
        LPDIRECTDRAWCLIPPER lpDDClipper;    // Clipper-Object
        LPDIRECTDRAWPALETTE lpDDPal;        // Palette Object

        unsigned long back_w,back_h;        // entspricht r->Width,r->Height
        void *back_ptr;
        unsigned long cur_ptr_type;         // nur fuer Vergleiche
        unsigned long back_pitch;
        unsigned long forcesoftcursor;
        unsigned long movieplayer;
        unsigned long paintmode;
        unsigned long forcealphatextures;
        unsigned long usedrawprimitive;
        unsigned long disablelowres;
        unsigned long exportwindowmode;

        unsigned long id;                   // momentan eingestellte "Display-ID"
        unsigned long flags;                // siehe unten
        void *r;                            // VFMBitmap entspricht lpDDSBack(!)

        struct wdd_D3D *d3d;                // dynamisch allokiert!
    };

    /*** DIVERSE SUPPORT-MACROS (aus dxsdk/d3dmacs.h) ***/
    #ifndef __cplusplus
    #define MAKE_MATRIX(lpDev, handle, data) \
        if (lpDev->lpVtbl->CreateMatrix(lpDev, &handle) != D3D_OK) \
            return FALSE; \
        if (lpDev->lpVtbl->SetMatrix(lpDev, handle, &data) != D3D_OK) \
            return FALSE
    #define RELEASE(x) if (x != NULL) {x->lpVtbl->Release(x); x = NULL;}
    #endif

    #ifdef __cplusplus
    #define MAKE_MATRIX(lpDev, handle, data) \
        if (lpDev->CreateMatrix(&handle) != D3D_OK) \
            return FALSE; \
        if (lpDev->SetMatrix(handle, &data) != D3D_OK) \
            return FALSE
    #define RELEASE(x) if (x != NULL) {x->Release(); x = NULL;}
    #endif

    #define PUTD3DINSTRUCTION(op, sz, cnt, ptr) \
        ((LPD3DINSTRUCTION) ptr)->bOpcode = op; \
        ((LPD3DINSTRUCTION) ptr)->bSize = sz; \
        ((LPD3DINSTRUCTION) ptr)->wCount = cnt; \
        ptr = (void *)(((LPD3DINSTRUCTION) ptr) + 1)

    #define VERTEX_DATA(loc, cnt, ptr) \
        if ((ptr) != (loc)) memcpy((ptr), (loc), sizeof(D3DVERTEX) * (cnt)); \
        ptr = (void *)(((LPD3DVERTEX) (ptr)) + (cnt))

    // OP_MATRIX_MULTIPLY size: 4 (sizeof D3DINSTRUCTION)
    #define OP_MATRIX_MULTIPLY(cnt, ptr) \
        PUTD3DINSTRUCTION(D3DOP_MATRIXMULTIPLY, sizeof(D3DMATRIXMULTIPLY), cnt, ptr)

    // MATRIX_MULTIPLY_DATA size: 12 (sizeof MATRIXMULTIPLY)
    #define MATRIX_MULTIPLY_DATA(src1, src2, dest, ptr) \
        ((LPD3DMATRIXMULTIPLY) ptr)->hSrcMatrix1 = src1; \
        ((LPD3DMATRIXMULTIPLY) ptr)->hSrcMatrix2 = src2; \
        ((LPD3DMATRIXMULTIPLY) ptr)->hDestMatrix = dest; \
        ptr = (void *)(((LPD3DMATRIXMULTIPLY) ptr) + 1)

    // OP_STATE_LIGHT size: 4 (sizeof D3DINSTRUCTION)
    #define OP_STATE_LIGHT(cnt, ptr) \
        PUTD3DINSTRUCTION(D3DOP_STATELIGHT, sizeof(D3DSTATE), cnt, ptr)

    // OP_STATE_TRANSFORM size: 4 (sizeof D3DINSTRUCTION)
    #define OP_STATE_TRANSFORM(cnt, ptr) \
        PUTD3DINSTRUCTION(D3DOP_STATETRANSFORM, sizeof(D3DSTATE), cnt, ptr)

    // OP_STATE_RENDER size: 4 (sizeof D3DINSTRUCTION)
    #define OP_STATE_RENDER(cnt, ptr) \
        PUTD3DINSTRUCTION(D3DOP_STATERENDER, sizeof(D3DSTATE), cnt, ptr)

    // STATE_DATA size: 8 (sizeof D3DSTATE)
    #define STATE_DATA(type, arg, ptr) \
        ((LPD3DSTATE) ptr)->drstRenderStateType = (D3DRENDERSTATETYPE)type; \
        ((LPD3DSTATE) ptr)->dwArg[0] = arg; \
        ptr = (void *)(((LPD3DSTATE) ptr) + 1)

    // OP_PROCESS_VERTICES size: 4 (sizeof D3DINSTRUCTION)
    #define OP_PROCESS_VERTICES(cnt, ptr) \
        PUTD3DINSTRUCTION(D3DOP_PROCESSVERTICES, sizeof(D3DPROCESSVERTICES), cnt, ptr)

    // PROCESSVERTICES_DATA size: 16 (sizeof D3DPROCESSVERTICES)
    #define PROCESSVERTICES_DATA(flgs, strt, cnt, ptr) \
        ((LPD3DPROCESSVERTICES) ptr)->dwFlags = flgs; \
        ((LPD3DPROCESSVERTICES) ptr)->wStart = strt; \
        ((LPD3DPROCESSVERTICES) ptr)->wDest = strt; \
        ((LPD3DPROCESSVERTICES) ptr)->dwCount = cnt; \
        ((LPD3DPROCESSVERTICES) ptr)->dwReserved = 0; \
        ptr = (void *)(((LPD3DPROCESSVERTICES) ptr) + 1)

    // OP_TRIANGLE_LIST size: 4 (sizeof D3DINSTRUCTION)
    #define OP_TRIANGLE_LIST(cnt, ptr) \
        PUTD3DINSTRUCTION(D3DOP_TRIANGLE, sizeof(D3DTRIANGLE), cnt, ptr)

    #define TRIANGLE_LIST_DATA(loc, count, ptr) \
        if ((ptr) != (loc)) memcpy((ptr), (loc), sizeof(D3DTRIANGLE) * (count)); \
        ptr = (void *)(((LPD3DTRIANGLE) (ptr)) + (count))

    // OP_LINE_LIST size: 4 (sizeof D3DINSTRUCTION)
    #define OP_LINE_LIST(cnt, ptr) \
        PUTD3DINSTRUCTION(D3DOP_LINE, sizeof(D3DLINE), cnt, ptr)

    #define LINE_LIST_DATA(loc, count, ptr) \
        if ((ptr) != (loc)) memcpy((ptr), (loc), sizeof(D3DLINE) * (count)); \
        ptr = (void *)(((LPD3DLINE) (ptr)) + (count))

    // OP_POINT_LIST size: 8 (sizeof D3DINSTRUCTION + sizeof D3DPOINT)
    #define OP_POINT_LIST(first, cnt, ptr) \
        PUTD3DINSTRUCTION(D3DOP_POINT, sizeof(D3DPOINT), 1, ptr); \
        ((LPD3DPOINT)(ptr))->wCount = cnt; \
        ((LPD3DPOINT)(ptr))->wFirst = first; \
        ptr = (void*)(((LPD3DPOINT)(ptr)) + 1)

    // OP_SPAN_LIST size: 8 (sizeof D3DINSTRUCTION + sizeof D3DSPAN)
    #define OP_SPAN_LIST(first, cnt, ptr) \
        PUTD3DINSTRUCTION(D3DOP_SPAN, sizeof(D3DSPAN), 1, ptr); \
        ((LPD3DSPAN)(ptr))->wCount = cnt; \
        ((LPD3DSPAN)(ptr))->wFirst = first; \
        ptr = (void*)(((LPD3DSPAN)(ptr)) + 1)

    // OP_BRANCH_FORWARD size: 18 (sizeof D3DINSTRUCTION + sizeof D3DBRANCH)
    #define OP_BRANCH_FORWARD(tmask, tvalue, tnegate, toffset, ptr) \
        PUTD3DINSTRUCTION(D3DOP_BRANCHFORWARD, sizeof(D3DBRANCH), 1, ptr); \
        ((LPD3DBRANCH) ptr)->dwMask = tmask; \
        ((LPD3DBRANCH) ptr)->dwValue = tvalue; \
        ((LPD3DBRANCH) ptr)->bNegate = tnegate; \
        ((LPD3DBRANCH) ptr)->dwOffset = toffset; \
        ptr = (void *)(((LPD3DBRANCH) (ptr)) + 1)

    // OP_SET_STATUS size: 20 (sizeof D3DINSTRUCTION + sizeof D3DSTATUS)
    #define OP_SET_STATUS(flags, status, _x1, _y1, _x2, _y2, ptr) \
        PUTD3DINSTRUCTION(D3DOP_SETSTATUS, sizeof(D3DSTATUS), 1, ptr); \
        ((LPD3DSTATUS)(ptr))->dwFlags = flags; \
        ((LPD3DSTATUS)(ptr))->dwStatus = status; \
        ((LPD3DSTATUS)(ptr))->drExtent.x1 = _x1; \
        ((LPD3DSTATUS)(ptr))->drExtent.y1 = _y1; \
        ((LPD3DSTATUS)(ptr))->drExtent.x2 = _x2; \
        ((LPD3DSTATUS)(ptr))->drExtent.y2 = _y2; \
        ptr = (void *)(((LPD3DSTATUS) (ptr)) + 1)

    // OP_NOP size: 4
    #define OP_NOP(ptr) \
        PUTD3DINSTRUCTION(D3DOP_TRIANGLE, sizeof(D3DTRIANGLE), 0, ptr)

    #define OP_EXIT(ptr) \
        PUTD3DINSTRUCTION(D3DOP_EXIT, 0, 0, ptr)

    #define QWORD_ALIGNED(ptr) \
        !(0x00000007L & (ULONG)(ptr))
#endif

/*** windd_data.flags ***/
#define WINDDF_IsWindowed       (1<<0)  // läuft in einem Window-Modus
#define WINDDF_Is8BppCLUT       (1<<1)  // Desktop ist 8 bpp CLUT
#define WINDDF_IsSysMem         (1<<2)  // Backbuffer ist im System-Mem
#define WINDDF_IsFullScrHalf    (1<<3)  // Fullscreen, halbierter Backbuffer
#define WINDDF_IsDirect3D       (1<<4)  // initialisiert für Direct3D

/*** wdd_EnableGDI()/wdd_DisableGDI() Flags ***/
#define WINDD_GDIMODE_WINDOW    (0)
#define WINDD_GDIMODE_MOVIE     (1)

/*-----------------------------------------------------------------*/
#endif

