#ifndef MISC_DBCS_H
#define MISC_DBCS_H
/*
**  $Source:$
**  $Revision:$
**  $Date:$
**  $Locker:$
**  $Author:$
**
**      misc/dbcssupp.h -- Linklib fuer DBCS Support. Windows Gummizelle.       
**
**  (C) Copyright 1997 by A.Weissflog
*/
#ifdef DBCS_PADDEDCELL
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


    /*** ein Puffer-Item ***/
    struct dbcs_Item {
            char *text;
            long x,y,w,h;
            long flags;
    };                                                                              

    /*** main data structure ***/
    #define DBCS_MAXNUM_ITEMS       (64)
    struct dbcs_Handle {
        LPDIRECTDRAW        lpDD;           // DirectDraw Object
        LPDIRECTDRAWSURFACE lpSurf;         // Surface in die gerendert wird
        HFONT               hFont;          // Font-Object
        long                Height;         // Fonthoehe in Pixel
        HDC                 hDC;            // DeviceContext von lpSurf
        unsigned long       ActItem;
        struct dbcs_Item    ItemBuf[DBCS_MAXNUM_ITEMS];
        unsigned long       ActRGB;
    };
#endif

/*** alignment modes for dbcs_AddChar ***/
#define DBCSF_LEFTALIGN     (1<<0)
#define DBCSF_RIGHTALIGN    (1<<1)
#define DBCSF_CENTER        (1<<2)
#define DBCSF_RELWIDTH      (1<<3)
#define DBCSF_PRESSED       (1<<4)  // in diesem Fall etwas versetzt zeichnen
#define DBCSF_COLOR         (1<<5)  // es handelt sich um eine Farb-Definition (x,y,w) == (r,g,b)

/*** function prototypes ***/
unsigned long dbcs_SetFont(char *font_def);
void dbcs_Kill(void);
char *dbcs_NextChar(char *);

#ifdef DBCS_PADDEDCELL
    unsigned long dbcs_Init(LPDIRECTDRAW lpdd,LPDIRECTDRAWSURFACE lpsurf,char *font_desc);
    unsigned long dbcs_Flush(DDSURFACEDESC *ddsd);
#endif
void dbcs_Add(char *text, long x, long y, long w, long h, unsigned long flags);

/*-----------------------------------------------------------------*/
#endif
