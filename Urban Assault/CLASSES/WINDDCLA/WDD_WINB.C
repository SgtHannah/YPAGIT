/*
**  $Source: PRG:VFM/Classes/_WinDDClass/wdd_winbox.c,v $
**  $Revision: 38.6 $
**  $Date: 1998/01/06 15:05:09 $
**  $Locker: floh $
**  $Author: floh $
**
**  Dies ist die speziell vor Typ-Kollisionen abgeschirmte
**  "WinBox" für die windd.class.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include "stdlib.h"
#include "string.h"
#include "memory.h"
#include "stdio.h"

#define WINDD_WINBOX
#define INITGUID
#include "bitmap/winddclass.h"
#include "imm.h"
#include "misc/dshow.h"

/*** externe Funktionen ***/
void wdd_AddDisplayMode(unsigned long w, unsigned long h, unsigned long bpp, unsigned long flags);
void wdd_AddDevice(char *name, char *guid, unsigned long is_current);

/*** interne Funktionen ***/
void wdd_KillDDrawObjects(struct windd_data *);
void wdd_SetMouseImage(struct windd_data *wdd, unsigned long type, unsigned long flush);

/*** globale Object-Handles ***/
LPDIRECTDRAW lpDD = NULL;
LPDIRECT3D2  lpD3D2 = NULL;

/*** globale Preferences-Variablen ***/
unsigned long wdd_DoDirect3D         = FALSE;  // initialisiere Direct3D-Umgebung
unsigned long wdd_CanDoAlpha         = FALSE;  // initialisiert von wdd_DDrawCreate()
unsigned long wdd_CanDoAdditiveBlend = FALSE;  // ditto
unsigned long wdd_CanDoStipple       = FALSE;  // ditto
unsigned long wdd_ForceAlphaTextures = FALSE;

/*** anderes globales Gesindel ***/
char *EnumDDrawName = NULL;
char *EnumDDrawDesc = NULL;
GUID *EnumDDrawGUID = NULL;

#define WDD_WINDOW_CLASS ("UA Window Class")   // don't change!

/*** definiert in win_main.c, ausgefuellt einmalig hier ***/
extern HWND win_HWnd;

/*** definiert und initialisiert in win_main.c ***/
extern HINSTANCE win_Instance;
extern int       win_CmdShow;

/*** globale Verwaltungs-Struktur ***/
struct wdd_Data wdd_Data;
HIMC win_HImc = NULL;      // Input Method Editor Context
unsigned long wdd_MoviePlaying = FALSE;

unsigned long wdd_DuringEnableGDI = FALSE;

/*** aus wdd_log.c ***/
void wdd_Log(char *string,...);

void wdd_Kill3D(struct windd_data *wdd);

/*-----------------------------------------------------------------*/
void wdd_FailMsg(char *title, char *msg, unsigned long code)
/*
**  FUNCTION
**      Generiert eine DirectDraw-Fehler-Messagebox.
**
**  CHANGED
**      14-Feb-97   floh    created
**      07-Mar-97   floh    erweitert um Direct3D-Fehlercodes
**      05-Mar-98   floh    + benutzt nicht mehr MessageBox,
**                            sondern wdd_Log.
*/
{
    char buf[128];
    char *err;

    if (code == DDERR_ALREADYINITIALIZED)               err = "AlreadyInitialized";
    else if (code == DDERR_BLTFASTCANTCLIP)             err = "CltFastCantClip";
    else if (code == DDERR_CANNOTATTACHSURFACE)         err = "CannotAttachSurface";
    else if (code == DDERR_CANNOTDETACHSURFACE)         err = "CannotDetachSurface";
    else if (code == DDERR_CANTCREATEDC)                err = "CantCreateDC";
    else if (code == DDERR_CANTDUPLICATE)               err = "CantDuplicate";
    else if (code == DDERR_CANTLOCKSURFACE)             err = "CantLockSurface";
    else if (code == DDERR_CANTPAGELOCK)                err = "CantPageLock";
    else if (code == DDERR_CANTPAGEUNLOCK)              err = "CantPageUnlock";
    else if (code == DDERR_CLIPPERISUSINGHWND)          err = "ClipperIsUsingHwnd";
    else if (code == DDERR_COLORKEYNOTSET)              err = "ColorKeyNotSet";
    else if (code == DDERR_CURRENTLYNOTAVAIL)           err = "CurrentlyNotAvail";
    else if (code == DDERR_DCALREADYCREATED)            err = "DCAlreadyCreated";
    else if (code == DDERR_DIRECTDRAWALREADYCREATED)    err = "DirectDrawAlreadyCreated";
    else if (code == DDERR_EXCEPTION)                   err = "Exception";
    else if (code == DDERR_EXCLUSIVEMODEALREADYSET)     err = "ExclusiveModeAlreadySet";
    else if (code == DDERR_GENERIC)                     err = "Generic";
    else if (code == DDERR_HEIGHTALIGN)                 err = "HeightAlign";
    else if (code == DDERR_HWNDALREADYSET)              err = "HwndAlreadySet";
    else if (code == DDERR_HWNDSUBCLASSED)              err = "HwndSubClassed";
    else if (code == DDERR_IMPLICITLYCREATED)           err = "ImplicitlyCreated";
    else if (code == DDERR_INCOMPATIBLEPRIMARY)         err = "IncompatiblePrimary";
    else if (code == DDERR_INVALIDCAPS)                 err = "InvalidCaps";
    else if (code == DDERR_INVALIDCLIPLIST)             err = "InvalidClipList";
    else if (code == DDERR_INVALIDDIRECTDRAWGUID)       err = "InvalidDirectDrawGUID";
    else if (code == DDERR_INVALIDMODE)                 err = "InvalidMode";
    else if (code == DDERR_INVALIDOBJECT)               err = "InvalidObject";
    else if (code == DDERR_INVALIDPARAMS)               err = "InvalidParams";
    else if (code == DDERR_INVALIDPIXELFORMAT)          err = "InvalidPixelFormat";
    else if (code == DDERR_INVALIDPOSITION)             err = "InvalidPosition";
    else if (code == DDERR_INVALIDRECT)                 err = "InvalidRect";
    else if (code == DDERR_INVALIDSURFACETYPE)          err = "InvalidSurfaceType";
    else if (code == DDERR_LOCKEDSURFACES)              err = "LockedSurfaces";
    else if (code == DDERR_NO3D)                        err = "No3D";
    else if (code == DDERR_NOALPHAHW)                   err = "NoAlphaHW";
    else if (code == DDERR_NOBLTHW)                     err = "NoBltHW";
    else if (code == DDERR_NOCLIPLIST)                  err = "NoClipList";
    else if (code == DDERR_NOCLIPPERATTACHED)           err = "NoClipperAttached";
    else if (code == DDERR_NOCOLORCONVHW)               err = "NoColorConvHW";
    else if (code == DDERR_NOCOLORKEY)                  err = "NoColorKey";
    else if (code == DDERR_NOCOLORKEYHW)                err = "NoColorKeyHW";
    else if (code == DDERR_NOCOOPERATIVELEVELSET)       err = "NoCooperativeLevelSet";
    else if (code == DDERR_NODC)                        err = "NoDC";
    else if (code == DDERR_NODDROPSHW)                  err = "NoDDRopsHW";
    else if (code == DDERR_NODIRECTDRAWHW)              err = "NoDirectDrawHW";
    else if (code == DDERR_NODIRECTDRAWSUPPORT)         err = "NoDirectDrawSupport";
    else if (code == DDERR_NOEMULATION)                 err = "NoEmulation";
    else if (code == DDERR_NOEXCLUSIVEMODE)             err = "NoExclusiveMode";
    else if (code == DDERR_NOFLIPHW)                    err = "NoFlipHW";
    else if (code == DDERR_NOGDI)                       err = "NoGDI";
    else if (code == DDERR_NOMIPMAPHW)                  err = "NoMipMapHW";
    else if (code == DDERR_NOMIRRORHW)                  err = "NoMirrorHW";
    else if (code == DDERR_NOOVERLAYDEST)               err = "NoOverlayDest";
    else if (code == DDERR_NOOVERLAYHW)                 err = "NoOverlayHW";
    else if (code == DDERR_NOPALETTEATTACHED)           err = "NoPaletteAttached";
    else if (code == DDERR_NOPALETTEHW)                 err = "NoPaletteHW";
    else if (code == DDERR_NORASTEROPHW)                err = "NoRasterOpHW";
    else if (code == DDERR_NOROTATIONHW)                err = "NoRotationHW";
    else if (code == DDERR_NOSTRETCHHW)                 err = "NoStretchHW";
    else if (code == DDERR_NOT4BITCOLOR)                err = "Not4BitColor";
    else if (code == DDERR_NOT4BITCOLORINDEX)           err = "Not4BitColorIndex";
    else if (code == DDERR_NOT8BITCOLOR)                err = "Not8BitColor";
    else if (code == DDERR_NOTAOVERLAYSURFACE)          err = "NotAOverlaySurface";
    else if (code == DDERR_NOTEXTUREHW)                 err = "NoTextureHW";
    else if (code == DDERR_NOTFLIPPABLE)                err = "NotFlippable";
    else if (code == DDERR_NOTFOUND)                    err = "NotFound";
    else if (code == DDERR_NOTINITIALIZED)              err = "NotInitialized";
    else if (code == DDERR_NOTLOCKED)                   err = "NotLocked";
    else if (code == DDERR_NOTPAGELOCKED)               err = "NotPageLocked";
    else if (code == DDERR_NOTPALETTIZED)               err = "NotPalettized";
    else if (code == DDERR_NOVSYNCHW)                   err = "NoVSyncHW";
    else if (code == DDERR_NOZBUFFERHW)                 err = "NoZBufferHW";
    else if (code == DDERR_NOZOVERLAYHW)                err = "NoZOverlayHW";
    else if (code == DDERR_OUTOFCAPS)                   err = "OutOfCaps";
    else if (code == DDERR_OUTOFMEMORY)                 err = "OutOfMemory";
    else if (code == DDERR_OUTOFVIDEOMEMORY)            err = "OutOfVideoMemory";
    else if (code == DDERR_OVERLAYCANTCLIP)             err = "OverlayCantClip";
    else if (code == DDERR_OVERLAYCOLORKEYONLYONEACTIVE)    err = "OverlayColorKeyOnlyOneActive";
    else if (code == DDERR_OVERLAYNOTVISIBLE)           err = "OverlayNotVisible";
    else if (code == DDERR_PALETTEBUSY)                 err = "PaletteBusy";
    else if (code == DDERR_PRIMARYSURFACEALREADYEXISTS) err = "PrimarySurfaceAlreadyExists";
    else if (code == DDERR_REGIONTOOSMALL)              err = "RegionTooSmall";
    else if (code == DDERR_SURFACEALREADYATTACHED)      err = "SurfaceAlreadyAttached";
    else if (code == DDERR_SURFACEALREADYDEPENDENT)     err = "SurfaceAlreadyDependent";
    else if (code == DDERR_SURFACEBUSY)                 err = "SurfaceBusy";
    else if (code == DDERR_SURFACEISOBSCURED)           err = "SurfaceIsObscured";
    else if (code == DDERR_SURFACELOST)                 err = "SurfaceLost";
    else if (code == DDERR_SURFACENOTATTACHED)          err = "SurfaceNotAttached";
    else if (code == DDERR_TOOBIGHEIGHT)                err = "TooBigHeight";
    else if (code == DDERR_TOOBIGSIZE)                  err = "TooBigSize";
    else if (code == DDERR_TOOBIGWIDTH)                 err = "TooBigWidth";
    else if (code == DDERR_UNSUPPORTED)                 err = "Unsupported";
    else if (code == DDERR_UNSUPPORTEDFORMAT)           err = "UnsupportedFormat";
    else if (code == DDERR_UNSUPPORTEDMASK)             err = "UnsupportedMask";
    else if (code == DDERR_UNSUPPORTEDMODE)             err = "UnsupportedMode";
    else if (code == DDERR_VERTICALBLANKINPROGRESS)     err = "VerticalBlankInProgress";
    else if (code == DDERR_WASSTILLDRAWING)             err = "WasStillDrawing";
    else if (code == DDERR_WRONGMODE)                   err = "WrongMode";
    else if (code == DDERR_XALIGN)                      err = "XAlign";
    else if (code == D3DERR_BADMAJORVERSION)            err = "BadMajorVersion";
    else if (code == D3DERR_BADMINORVERSION)            err = "BadMinorVersion";
    else if (code == D3DERR_EXECUTE_CREATE_FAILED)      err = "ExecuteCreateFailed";
    else if (code == D3DERR_EXECUTE_DESTROY_FAILED)     err = "ExecuteDestroyFailed";
    else if (code == D3DERR_EXECUTE_LOCK_FAILED)        err = "ExecuteLockFailed";
    else if (code == D3DERR_EXECUTE_UNLOCK_FAILED)      err = "ExecuteUnlockFailed";
    else if (code == D3DERR_EXECUTE_NOT_LOCKED)         err = "ExecuteNotLocked";
    else if (code == D3DERR_EXECUTE_FAILED)             err = "ExecuteFailed";
    else if (code == D3DERR_EXECUTE_CLIPPED_FAILED)     err = "ExecuteClippedFailed";
    else if (code == D3DERR_TEXTURE_NO_SUPPORT)         err = "TextureNoSupport";
    else if (code == D3DERR_TEXTURE_CREATE_FAILED)      err = "TextureCreateFailed";
    else if (code == D3DERR_TEXTURE_DESTROY_FAILED)     err = "TextureDestroyFailed";
    else if (code == D3DERR_TEXTURE_LOCK_FAILED)        err = "TextureLockFailed";
    else if (code == D3DERR_TEXTURE_UNLOCK_FAILED)      err = "TextureUnlockFailed";
    else if (code == D3DERR_TEXTURE_LOAD_FAILED)        err = "TextureUnlockFailed";
    else if (code == D3DERR_TEXTURE_SWAP_FAILED)        err = "TextureSwapFailed";
    else if (code == D3DERR_TEXTURE_LOCKED)             err = "TextureLocked";
    else if (code == D3DERR_TEXTURE_NOT_LOCKED)         err = "TextureNotLocked";
    else if (code == D3DERR_TEXTURE_GETSURF_FAILED)     err = "TextureGetSurfFailed";
    else if (code == D3DERR_MATRIX_CREATE_FAILED)       err = "MatrixCreateFailed";
    else if (code == D3DERR_MATRIX_DESTROY_FAILED)      err = "MatrixDestroyFailed";
    else if (code == D3DERR_MATRIX_SETDATA_FAILED)      err = "MatrixSetDataFailed";
    else if (code == D3DERR_MATRIX_GETDATA_FAILED)      err = "MatrixGetDataFailed";
    else if (code == D3DERR_SETVIEWPORTDATA_FAILED)     err = "SetViewportDataFailed";
    else if (code == D3DERR_MATERIAL_CREATE_FAILED)     err = "MaterialCreateFailed";
    else if (code == D3DERR_MATERIAL_DESTROY_FAILED)    err = "MaterialDestroyFailed";
    else if (code == D3DERR_MATERIAL_SETDATA_FAILED)    err = "MaterialSetDataFailed";
    else if (code == D3DERR_MATERIAL_GETDATA_FAILED)    err = "MaterialGetDataFailed";
    else if (code == D3DERR_LIGHT_SET_FAILED)           err = "LightSetFailed";
    else if (code == D3DERR_SCENE_IN_SCENE)             err = "SceneInScene";
    else if (code == D3DERR_SCENE_NOT_IN_SCENE)         err = "SceneNotInScene";
    else if (code == D3DERR_SCENE_BEGIN_FAILED)         err = "SceneBeginFailed";
    else if (code == D3DERR_SCENE_END_FAILED)           err = "SceneEndFailed";
    else err = "";
    wdd_Log("FAIL MSG: title=%s, msg=%s, err=%s\n",title,msg,err);
}

/*-----------------------------------------------------------------*/
void wdd_Guid2Str(char *buf, GUID *guid, char *alt_text)
/*
**  FUNCTION
**      Schreibt die GUID in einen String-Buffer.
**
**  CHANGED
**      11-Mar-98   floh    created
*/
{
    if (guid) {
        sprintf(buf,"0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x",
                    guid->Data1,guid->Data2,guid->Data3,
                    guid->Data4[0],guid->Data4[1],guid->Data4[2],guid->Data4[3],
                    guid->Data4[4],guid->Data4[5],guid->Data4[6],guid->Data4[7]);
    } else {
        strcpy(buf,alt_text);
    };
}

/*-----------------------------------------------------------------*/
unsigned long wdd_ReadGUID(char *fname, GUID *guid)
/*
**  FUNCTION
**      Liest eine GUID aus einem File aus.
**
**  RESULTS
**      0   -> oops, ein Fehler
**      1   -> alles OK, GUID ist gültig
**      2   -> es stand der Text <primary> im File
**      3   -> es stand der Text <software> im File
**
**  CHANGED
**      10-Mar-98   floh    created
**      11-Mar-98   floh    erweitert um <primary> und <software>
*/
{
    FILE *fp;
    unsigned long retval = 0;
    memset(guid,0,sizeof(GUID));
    if (fp = fopen(fname,"r")) {
        char buf[256];
        char *dike_out;
        char *tok;
        unsigned long i;
        fgets(buf,sizeof(buf),fp);
        fclose(fp);
        if (dike_out = strpbrk(buf,"\n;")) *dike_out=0;
        if (strcmp(buf,"<primary>") == 0)        retval = 2;
        else if (strcmp(buf,"<software>") == 0) retval = 3;
        else {
            tok = strtok(buf,", \t");
            if (tok) guid->Data1 = (unsigned long) strtoul(tok,NULL,0);
            tok = strtok(NULL,", \t");
            if (tok) guid->Data2 = (unsigned short) strtoul(tok,NULL,0);
            tok = strtok(NULL,", \t");
            if (tok) guid->Data3 = (unsigned short) strtoul(tok,NULL,0);
            for (i=0; i<8; i++) {
                tok = strtok(NULL,", \t");
                if (tok) guid->Data4[i] = (unsigned char) strtoul(tok,NULL,0);
            };
            retval = 1;
        };
    };
    return(retval);
}

/*-----------------------------------------------------------------*/
unsigned long wdd_WriteGUID(char *fname, GUID *guid, char *alt_text)
/*
**  FUNCTION
**      Schreibt eine GUID in einen File.
**      Falls der <guid> Pointer NULL ist, wird <alt_text>
**      in den File geschrieben
**
**  CHANGED
**      10-Mar-98   floh    created
*/
{
    FILE *fp;
    unsigned long retval = 0;
    if (fp = fopen(fname,"w")) {
        char buf[128];
        wdd_Guid2Str(buf,guid,alt_text);
        fputs(buf,fp);
        retval = 1;
        fclose(fp);
    };
    return(retval);
}

/*-----------------------------------------------------------------*/
void wdd_CheckLostSurfaces(struct windd_data *wdd)
/*
**  FUNCTION
**      Testet ALLE fürs Rendering benötigten Surfaces auf
**      Lost-Status und restored sie ggfls.
**
**  CHANGED
**      08-Mar-97   floh    created
*/
{
    HRESULT ddrval;

    /*** Lost->Restore ***/
    if (wdd->lpDDSPrim) {
        ddrval = wdd->lpDDSPrim->lpVtbl->IsLost(wdd->lpDDSPrim);
        if (ddrval != DD_OK) {
            ddrval = wdd->lpDDSPrim->lpVtbl->Restore(wdd->lpDDSPrim);
        };
    };
    if (wdd->lpDDSBack) {
        ddrval = wdd->lpDDSBack->lpVtbl->IsLost(wdd->lpDDSBack);
        if (ddrval != DD_OK) {
            ddrval = wdd->lpDDSBack->lpVtbl->Restore(wdd->lpDDSBack);
        };
    };
    if (wdd->lpDDSBackup) {
        ddrval = wdd->lpDDSBackup->lpVtbl->IsLost(wdd->lpDDSBackup);
        if (ddrval != DD_OK) {
            ddrval = wdd->lpDDSBackup->lpVtbl->Restore(wdd->lpDDSBackup);
        };
    };
    if (wdd->d3d) {
        if (wdd->d3d->lpDDSZBuffer) {
            ddrval = wdd->d3d->lpDDSZBuffer->lpVtbl->IsLost(wdd->d3d->lpDDSZBuffer);
            if (ddrval != DD_OK) {
                ddrval = wdd->d3d->lpDDSZBuffer->lpVtbl->Restore(wdd->d3d->lpDDSZBuffer);
            };
        };
    };
}

/*-----------------------------------------------------------------*/
HRESULT wdd_CreateBackupSurface(struct windd_data *wdd)
/*
**  FUNCTION
**      Erzeugt eine Backupsurface als Kopie der Backsurface
**      (nur im Sysmem) und kopiert deren Inhalt. Das
**      ganze für EnableGDI().
**
**  CHANGED
**      07-Jan-98   floh    created
**      27-Jan-98   floh    kopiert jetzt die PrimSurface
*/
{
    DDSURFACEDESC ddsd;
    HRESULT ddrval;

    /*** Lost-Restore ***/
    wdd_CheckLostSurfaces(wdd);

    /*** Surfacedesc der Backsurface holen ***/
    memset(&ddsd,0,sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddrval = wdd->lpDDSBack->lpVtbl->GetSurfaceDesc(wdd->lpDDSBack,&ddsd);
    if (DD_OK == ddrval) {
        DWORD w,h;
        w = ddsd.dwWidth;
        h = ddsd.dwHeight;

        /*** neue Surface im SysMem erzeugen ***/
        memset(&ddsd,0,sizeof(ddsd));
        ddsd.dwSize  = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH;
        ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN|DDSCAPS_SYSTEMMEMORY;
        ddsd.dwHeight = h;
        ddsd.dwWidth  = w;
        ddrval = lpDD->lpVtbl->CreateSurface(lpDD,&ddsd,&(wdd->lpDDSBackup),NULL);
        if (ddrval == DD_OK) {

            /*** falls existent, Palette attachen ***/
            if (wdd->lpDDPal) {
                wdd->lpDDSBackup->lpVtbl->SetPalette(wdd->lpDDSBackup,wdd->lpDDPal);
            };

            /*** kopiere den Inhalt der Primsurface in die Sysmem-Surface ***/
            wdd->lpDDSBackup->lpVtbl->Blt(wdd->lpDDSBackup,
                NULL, wdd->lpDDSPrim, NULL, DDBLT_WAIT, NULL);
        };
    };

    /*** das war's ***/
    return(ddrval);
}

/*-----------------------------------------------------------------*/
void wdd_DestroyBackupSurface(struct windd_data *wdd)
/*
**  FUNCTION
**      Killt die mit wdd_CreateBackupSurface() erzeugte
**      Surface wieder.
**
**  CHANGED
**      07-Jan-98   floh    created
*/
{
    if (wdd->lpDDSBackup) {
        wdd->lpDDSBackup->lpVtbl->Release(wdd->lpDDSBackup);
        wdd->lpDDSBackup = NULL;
    };
}

/*-----------------------------------------------------------------*/
void wdd_BltBgDC(struct windd_data *wdd)
/*
**  FUNCTION
**      Blittet den Inhalt der Backsurface per Windows-
**      Blit-Routinen in die Primary-Surface. Wird von
**      Enable-GDI aufgerufen.
**
**  CHANGED
**      07-Jan-97   floh    created
**      26-Feb-98   floh    kein CheckLostSurfaces mehr
*/
{
    if (wdd->hWnd && wdd->lpDDSBackup) {
        HDC hdcSrc,hdcDest;
        RECT rectSrc,rectDest;
        HRESULT ddrval;
        hdcDest = GetDC(wdd->hWnd);
        if (hdcDest) {
            ddrval = wdd->lpDDSBackup->lpVtbl->GetDC(wdd->lpDDSBackup,&hdcSrc);
            if (ddrval == DD_OK) {

                char pal_buf[256*4 + sizeof(LOGPALETTE)];
                HPALETTE hpal = NULL;
                LOGPALETTE *logpal = (LOGPALETTE *) pal_buf;
                PALETTEENTRY *pal  = &(logpal->palPalEntry[0]);

                if (!wdd_DoDirect3D) {
                    /*** erzeuge und attache Palette ***/
                    int i;
                    memset(pal_buf,0,sizeof(pal_buf));
                    logpal->palVersion    = 0x300;
                    logpal->palNumEntries = 256;
                    for (i=0; i<10; i++) {
                        pal[i].peFlags = pal[i+246].peFlags = PC_EXPLICIT;
                        pal[i].peRed = i;
                        pal[i+246].peRed = i+246;
                    };
                    if (wdd->lpDDPal) {
                        ddrval = wdd->lpDDPal->lpVtbl->GetEntries(wdd->lpDDPal,0,10,236,&(pal[10]));
                    };
                    hpal = CreatePalette(logpal);
                    if (hpal) {
                        SelectPalette(hdcDest,hpal,FALSE);
                        RealizePalette(hdcDest);
                    };
                };

                /*** Größe der Rects ausfüllen ***/
                GetClientRect(wdd->hWnd,&rectDest);
                rectSrc.left = 0;
                rectSrc.top  = 0;
                if (wdd->flags & WINDDF_IsFullScrHalf) {
                    rectSrc.right  = wdd->back_w>>1;
                    rectSrc.bottom = wdd->back_h>>1;
                } else {
                    rectSrc.right  = wdd->back_w;
                    rectSrc.bottom = wdd->back_h;
                };

                /*** und blitten ***/
                if ((rectDest.right  == rectSrc.right) &&
                    (rectDest.bottom == rectSrc.bottom))
                {
                    BitBlt(hdcDest,0,0,rectDest.right,rectDest.bottom,
                           hdcSrc,0,0,SRCCOPY);
                } else {
                    StretchBlt(hdcDest,0,0,rectDest.right,rectDest.bottom,
                               hdcSrc,0,0,rectSrc.right,rectSrc.bottom,
                               SRCCOPY);
                };

                /*** falls Palette existiert, löschen ***/
                if (hpal) {
                    DeleteObject(hpal);
                    hpal = NULL;
                };
                wdd->lpDDSBackup->lpVtbl->ReleaseDC(wdd->lpDDSBackup,hdcSrc);
            };
            ReleaseDC(wdd->hWnd,hdcDest);
        };
    };
}

/*-----------------------------------------------------------------*/
long FAR PASCAL wdd_WinProc(HWND hWnd, UINT message,
                            WPARAM wParam, LPARAM lParam)
/*
**  FUNCTION
**      Message-Loop für das Output-Fenster.
**
**  CHANGED
**      09-Nov-96   floh    created
**      15-Feb-97   floh    + WM_PAINT funktioniert wieder
**      08-Apr-97   floh    + WM_PAINT funktioniert auch mal wieder
**      14-Jul-97   floh    + bei WM_SIZE wird der WindowStyle auf
**                            "CAPTION|SYSMENU" geändert, damit im
**                            MINIMIZED Modus ein Icon sichtbar ist.
**      17-Aug-97   floh    + WM_PAINT löscht PrimSurface, wenn im
**                            EnableGDI Modus
**      19-Aug-97   floh    + schluckt jetzt WM_SYSCOMMAND/SCKEYMENU
**      22-Nov-97   floh    + diverse Blit-Routinen konnten ausgefuehrt werden,
**                            wenn die entsprechenden Surfaces noch gar nicht
**                            initialisiert waren...
**      23-Jan-98   floh    + WM_KEYDOWN unterbricht Movieplayer
**      05-Feb-98   floh    + reworked.
*/
{
    /*** hole LID des windd.class Objekts als "UserData" ***/
    struct windd_data *wdd = (struct windd_data *) GetClassLong(hWnd,0);

    switch(message) {
        case WM_ACTIVATE:
            if (wdd) wdd_SetMouseImage(wdd,1,TRUE);
            break;    
    
        case WM_ACTIVATEAPP:
            if (wdd) wdd_SetMouseImage(wdd,1,TRUE);
            break;

        case WM_ERASEBKGND:
            /*** Background-Löschen abwürgen ***/
            return(1);
            break;

        case WM_PAINT:
            {
                HDC hdc;
                PAINTSTRUCT ps;
                hdc = BeginPaint(hWnd,&ps);
                if (wdd && wdd->hWnd && wdd->lpDDSPrim && wdd->lpDDSBack) {
                    HRESULT ddrval;
                    if (!wdd_DuringEnableGDI) {
                        wdd_CheckLostSurfaces(wdd);
                        if (wdd->flags & WINDDF_IsWindowed) {
                            /*** Window-Mode: etwas mehr Arbeit ***/
                            RECT dest_r;
                            POINT pt;
                            GetClientRect(wdd->hWnd,&dest_r);
                            pt.x = 0;
                            pt.y = 0;
                            ClientToScreen(wdd->hWnd,&pt);
                            OffsetRect(&dest_r,pt.x,pt.y);
                            ddrval = wdd->lpDDSPrim->lpVtbl->Blt(wdd->lpDDSPrim,
                                     &dest_r, wdd->lpDDSBack, NULL, DDBLT_WAIT, NULL);
                        } else {
                            if (!wdd_DoDirect3D) {
                                /*** sonst Blit, weil in SysMem ***/
                                ddrval = wdd->lpDDSPrim->lpVtbl->Blt(wdd->lpDDSPrim,NULL,
                                         wdd->lpDDSBack,NULL,DDBLT_WAIT,NULL);
                            };
                        };
                    } else {
                        wdd_BltBgDC(wdd);
                    };
                };
                EndPaint(hWnd,&ps);
            };
            break;

        case WM_PALETTECHANGED:
            if ((HWND)wParam == wdd->hWnd) break;
            // Fall Through to WM_QUERYNEWPALETTE
        case WM_QUERYNEWPALETTE:
            /*** Palette muß neu installiert werden ***/
            if (wdd && wdd->lpDDPal) {
                wdd->lpDDSPrim->lpVtbl->SetPalette(wdd->lpDDSPrim,wdd->lpDDPal);
            };
            break;

        case WM_CREATE:
            /*** Window wurde erzeugt ***/
            break;

        case WM_CLOSE:
            /*** Window-Close requested, -> DefWindowProc() wird ***/
            /*** DestroyWindow() aufrufen                        ***/
            break;

        case WM_DESTROY:
            /*** DestroyWindow() wurde von irgendwoher aufgerufen   ***/
            /*** bevor das passiert, müssen die DirectDraw-Objects, ***/
            /*** die an dem Window-Handle hängen, gekillt werden    ***/
            if (wdd) {
                wdd_KillDDrawObjects(wdd);
                wdd->hWnd = NULL;
                PostQuitMessage(0);
            };
            if (win_HImc) {
                ImmAssociateContext(hWnd,win_HImc);
                win_HImc = NULL;
            };
            return(0);
            break;

        case WM_QUIT:
            /*** bewirkt, das GetMessage() mit 0 zurückkehrt ***/
            break;

        case WM_SYSCOMMAND:
            /*** KeyMenu schlucken ***/
            if (wParam == SC_KEYMENU) return(0);
            break;
    };

    /*** den Default-Handler aufrufen ***/
    return(DefWindowProc(hWnd,message,wParam,lParam));
}

/*-----------------------------------------------------------------*/
unsigned long wdd_DoSoftCursor(struct windd_data *wdd)
/*
**  CHANGED
**      14-Apr-97   floh    created
*/
{
    return((wdd_Data.Driver.DoSoftCursor)||(wdd->forcesoftcursor));
}

/*-----------------------------------------------------------------*/
unsigned long wdd_BPP2DDBD(unsigned long bpp)
/*
**  FUNCTION
**      Konvertiert Bits-Per-Pixel nach DirectDraw-Konstante.
**
**  CHANGED
**      05-Mar-97   floh    created
*/
{
    switch(bpp) {
        case 1:  return DDBD_1;
        case 2:  return DDBD_2;
        case 4:  return DDBD_4;
        case 8:  return DDBD_8;
        case 16: return DDBD_16;
        case 24: return DDBD_24;
        case 32: return DDBD_32;
        default: return 0;
    };
}

/*-----------------------------------------------------------------*/
HRESULT FAR PASCAL wdd_EnumDevicesCallback(
                    GUID FAR *lpGUID,
                    LPSTR lpDeviceDesc,
                    LPSTR lpDeviceName,
                    LPD3DDEVICEDESC lpHWDesc,
                    LPD3DDEVICEDESC lpHELDesc,
                    LPVOID lpUserArg)
/*
**  FUNCTION
**      Untersucht das angegebene Device auf Tauglichkeit.
**      Schreibt alle gefundenen Devices nach wdd_Data.AllDrivers[],
**      zählt dabei wdd_Data.NumDrivers hoch
**
**  CHANGED
**      04-Mar-98   floh    created
**      18-Mar-98   floh    + schreibt Videomem mit
**                          + ermittelt gleich hier die ZBuffer-BitDepth
*/
{
    struct wdd_3DDriver *d = &(wdd_Data.AllDrivers[wdd_Data.NumDrivers]);
    GUID FAR *lpDDrawGUID  = EnumDDrawGUID;
    memset(d,0,sizeof(struct wdd_3DDriver));

    wdd_Log("-> enum devices:\n");
    wdd_Log("    name = %s\n",lpDeviceName);
    wdd_Log("    desc = %s\n",lpDeviceDesc);

    /*** GUIDs mitschreiben ***/
    if (lpGUID) {
        memcpy(&(d->DevGuid),lpGUID,sizeof(GUID));
        d->DevGuidIsValid = TRUE;
        wdd_Log("    guid = 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n",
            lpGUID->Data1,lpGUID->Data2,lpGUID->Data3,
            lpGUID->Data4[0],lpGUID->Data4[1],lpGUID->Data4[2],lpGUID->Data4[3],
            lpGUID->Data4[4],lpGUID->Data4[5],lpGUID->Data4[6],lpGUID->Data4[7]);
    };
    if (lpDDrawGUID) {
        memcpy(&(d->DDrawGuid),lpDDrawGUID,sizeof(GUID));
        d->DDrawGuidIsValid = TRUE;
    };
    strncpy(&(d->DeviceName),lpDeviceName,sizeof(d->DeviceName)-1);
    strncpy(&(d->DeviceDesc),lpDeviceDesc,sizeof(d->DeviceDesc)-1);
    strncpy(&(d->DDrawName),EnumDDrawName,sizeof(d->DDrawName)-1);
    strncpy(&(d->DDrawDesc),EnumDDrawDesc,sizeof(d->DDrawDesc)-1);

    /*** ein Hardware-Device? ***/
    if (lpHWDesc->dcmColorModel) {
        d->IsHardware = TRUE;
        memcpy(&(d->Desc),lpHWDesc,sizeof(D3DDEVICEDESC));
        wdd_Log("enum devices: ok, is hardware\n");
    }else{
        wdd_Log("enum devices: skip, is not hardware\n");
        memset(d,0,sizeof(wdd_Data.Driver));
        return(D3DENUMRET_OK);
    };

    /*** kann in Desktop-Bittiefe rendern? ***/
    d->CanDoWindow  = FALSE;
    d->DoSoftCursor = FALSE;
    if (wdd_BPP2DDBD(wdd_Data.Desktop.ddpfPixelFormat.dwRGBBitCount) &
        d->Desc.dwDeviceRenderBitDepth)
    {
        d->CanDoWindow = TRUE;
        wdd_Log("enum devices: can render into desktop bit depth\n");
    };

    /*** arbeitet mit ZBuffer? ***/
    if (d->Desc.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_ZBUFFERLESSHSR)
    {
        d->DoesZBuffer = FALSE;
        wdd_Log("enum devices: zbufferlesshsr set (no zbuf used)\n");
    }else if (d->Desc.dwDeviceZBufferBitDepth){
        d->DoesZBuffer = TRUE;
        wdd_Log("enum devices: use zbuffer\n");
    }else{
        wdd_Log("enum devices: skip, no hidden surface removal...\n");
        return(D3DENUMRET_OK);
    };
    if (d->DoesZBuffer) {
        unsigned long depth = d->Desc.dwDeviceZBufferBitDepth;
        if (depth & DDBD_16)      d->ZBufBitDepth = 16;
        else if (depth & DDBD_8)  d->ZBufBitDepth = 8;
        else if (depth & DDBD_24) d->ZBufBitDepth = 24;
        else if (depth & DDBD_32) d->ZBufBitDepth = 32;
        wdd_Log("enum devices: zbuf bit depth is %d\n",d->ZBufBitDepth);
    };

    /*** welchen Alphablending Mode nehmen? ***/
    d->CanDoAlpha          = FALSE;
    d->CanDoAdditiveBlend  = FALSE;
    d->CanDoStipple        = FALSE;
    if ((d->Desc.dpcTriCaps.dwSrcBlendCaps  & D3DPBLENDCAPS_SRCALPHA) &&
        (d->Desc.dpcTriCaps.dwDestBlendCaps & D3DPBLENDCAPS_ONE) &&
        (!(wdd_ForceAlphaTextures)))
    {
        /*** der Idealfall ***/
        d->CanDoAlpha          = TRUE;
        d->CanDoAdditiveBlend  = TRUE;
        wdd_Log("enum devices: can do srcblend = srcalpha; destblend = one\n");
    }else if (d->Desc.dpcTriCaps.dwShadeCaps &
          (D3DPSHADECAPS_ALPHAFLATSTIPPLED|D3DPSHADECAPS_ALPHAGOURAUDSTIPPLED))
    {
        /*** kann nur stippeln ***/
        d->CanDoStipple  = TRUE;
        wdd_Log("enum devices: can do alpha stippling\n");
    }else if ((d->Desc.dpcTriCaps.dwSrcBlendCaps  & D3DPBLENDCAPS_SRCALPHA) &&
              (d->Desc.dpcTriCaps.dwDestBlendCaps & D3DPBLENDCAPS_INVSRCALPHA))
    {
        d->CanDoAlpha = TRUE;
        d->CanDoAdditiveBlend = FALSE;
        wdd_Log("enum devices: can do srcblend = srcalpha; destblend = invsrcalpha\n");
    }else{
        /*** sonst geht gar nix ***/
        wdd_Log("enum devices: skip, no alpha, no stipple...\n");
        return(D3DENUMRET_OK);
    };
    wdd_CanDoAlpha         = d->CanDoAlpha;
    wdd_CanDoAdditiveBlend = d->CanDoAdditiveBlend;
    wdd_CanDoStipple       = d->CanDoStipple;

    /*** Texturen im SysMem? ***/
    if (d->Desc.dwDevCaps & D3DDEVCAPS_TEXTURESYSTEMMEMORY) {
        d->CanDoSysMemTxt  = TRUE;
        wdd_Log("enum devices: can do sysmem textures\n");
    }else if (d->Desc.dwDevCaps & D3DDEVCAPS_TEXTUREVIDEOMEMORY){
        d->CanDoSysMemTxt = FALSE;
        wdd_Log("enum devices: textures in vmem\n");
    }else{
        wdd_Log("enum devices: skip, does not support texture mapping\n");
        return(D3DENUMRET_OK);
    };

    /*** klappt Colorkeying? ***/
    if (d->Desc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_TRANSPARENCY) {
        d->CanDoColorKeyTxt = TRUE;
        wdd_Log("enum devices: can do colorkey\n");
    }else{
        d->CanDoColorKeyTxt = FALSE;
        wdd_Log("enum devices: no colorkey support\n");
    };

    /*** wenn bis hierher gekommen, ist das Device perfekt... ***/
    wdd_Data.NumDrivers++;
    wdd_Log("enum devices: device accepted\n");
    return(D3DENUMRET_OK);
}

/*-----------------------------------------------------------------*/
BOOL FAR PASCAL wdd_DDrawEnumCallback(GUID FAR *lpGUID,
                                      LPSTR lpDriverDesc,
                                      LPSTR lpDriverName,
                                      LPVOID lpContext)
/*
**  FUNCTION
**      Sucht im D3D-Modus ein DDraw-Objekt raus, welches
**      für D3D geeignet ist.
**
**  CHANGED
**      03-Mar-97   floh    created
**      04-Mar-98   floh    macht auf jedes DDraw-Object ein
**                          EnumDevices
**      10-Mar-98   floh    + es wird nicht mehr beim 1. gefundenen
**                            Device abgebrochen, sondern es werden
**                            alle Devices im System enumeriert
*/
{
    DDCAPS driver_caps, hel_caps;
    HRESULT ddrval;
    char guid_buf[128];

    lpDD = NULL;
    lpD3D2 = NULL;
    if (DirectDrawCreate(lpGUID, &lpDD, NULL) != DD_OK) {
        return DDENUMRET_OK;
    };

    /*** Surface-Desk des Desktops besorgen ***/
    wdd_Data.Desktop.dwSize = sizeof(wdd_Data.Desktop);
    lpDD->lpVtbl->GetDisplayMode(lpDD,&(wdd_Data.Desktop));

    /*** Status-Ausgabe ***/
    wdd_Guid2Str(guid_buf,lpGUID,"<primary>");
    wdd_Log("-> enumerate ddraw objects...\n");
    wdd_Log("    -> name = %s\n",lpDriverName);
    wdd_Log("    -> desc = %s\n",lpDriverDesc);
    wdd_Log("    -> guid = %s\n",guid_buf);

    /*** Caps abfragen ***/
    memset(&driver_caps,0,sizeof(driver_caps));
    driver_caps.dwSize = sizeof(driver_caps);
    memset(&hel_caps,0,sizeof(hel_caps));
    hel_caps.dwSize = sizeof(hel_caps);
    if (lpDD->lpVtbl->GetCaps(lpDD,&driver_caps,&hel_caps) != DD_OK) {
        /*** Fehler, Driver ignorieren ***/
        lpDD->lpVtbl->Release(lpDD);
        lpDD = NULL;
        return(DDENUMRET_OK);
    };
    if (driver_caps.dwCaps & DDCAPS_3D) {
        /*** juhuu... Erfolg, ist aber nicht das Primary ***/
        LPDIRECT3D lpd3d = NULL;
        DDSURFACEDESC ddsd;

        /*** alle 3D Devices mitschreiben ***/
        ddrval = lpDD->lpVtbl->QueryInterface(lpDD,&IID_IDirect3D2,&lpD3D2);
        if (ddrval == DD_OK) {
            EnumDDrawName   = lpDriverName;
            EnumDDrawDesc   = lpDriverDesc;
            EnumDDrawGUID   = lpGUID;
            lpD3D2->lpVtbl->EnumDevices(lpD3D2,
                           (LPD3DENUMDEVICESCALLBACK)wdd_EnumDevicesCallback,
                           NULL);
            EnumDDrawName = NULL;
            EnumDDrawDesc = NULL;
            EnumDDrawGUID = NULL;

            /*** ab hier aufräumen ***/
            lpD3D2->lpVtbl->Release(lpD3D2);
            lpD3D2 = NULL;
        };
    };
    lpDD->lpVtbl->Release(lpDD);
    lpDD = NULL;
    return(DDENUMRET_OK);
}

/*-----------------------------------------------------------------*/
HRESULT WINAPI wdd_EnumDisplayModesCallback(LPDDSURFACEDESC lpddsd,
                                            LPVOID lpContext)
/*
**  FUNCTION
**      Callback-Routinen-Stub für EnumDisplayModes()
**      (also ausschließlich Fullscreen-Modes).
**
**  CHANGED
**      14-Nov-96   floh    created
**      03-Feb-97   floh    fuer jeden Modus eine Version mit halber
**                          Aufloesung
**      14-Feb-97   floh    fuer X-Auflösungen < 640 werden keine
**                          halbierten Auflösungen mehr angeboten
**      03-Mar-97   floh    füllt jetzt "nur" das globale Modes-
**                          Array aus.
**      05-Mar-97   floh    im Direct3D-Modus werden die Modes gegen
**                          das ausgewählte 3D-Device gefiltert.
**      06-Mar-97   floh    schneidet jetzt einfach alles mit,
**                          gefiltert wird später
**      03-Mar-98   floh    + logged die Displaymodes
*/
{
    /*** ignoriere Modes größer 1024x768 ***/
    if ((lpddsd->dwWidth>1024)||(lpddsd->dwHeight>768)) return(DDENUMRET_OK);

    /*** Mode mitschneiden ***/
    wdd_Data.Modes[wdd_Data.NumModes].w = lpddsd->dwWidth;
    wdd_Data.Modes[wdd_Data.NumModes].h = lpddsd->dwHeight;
    wdd_Data.Modes[wdd_Data.NumModes].bpp = lpddsd->ddpfPixelFormat.dwRGBBitCount;
    wdd_Data.Modes[wdd_Data.NumModes].can_do_3d = FALSE; // wird noch modifiziert
    wdd_Log("enum display mode: %dx%dx%d\n",lpddsd->dwWidth,lpddsd->dwHeight,lpddsd->ddpfPixelFormat.dwRGBBitCount);

    /*** Mode validieren ***/
    wdd_Data.NumModes++;
    if (wdd_Data.NumModes == WINDD_MAXNUMMODES) return(DDENUMRET_CANCEL);
    else                                        return(DDENUMRET_OK);
}

/*-----------------------------------------------------------------*/
HRESULT WINAPI wdd_EnumTextureFormatsCallback(LPDDSURFACEDESC lpDDSD,
                                              LPVOID lpContext)
/*
**  FUNCTION
**      Kaut alle Texturformate durch.
**
**  CHANGED
**      06-Mar-97   floh    created
**      20-Aug-97   floh    + Texturen mit Alphakanal werden jetzt
**                            akzeptiert
**      03-Mar-98   floh    + logged die Texturformate
*/
{
    struct windd_data *wdd = (struct windd_data *)lpContext;
    struct wdd_TxtFormat *t = &(wdd->d3d->TxtFormats[wdd->d3d->NumTxtFormats]);

    /*** DDSURFACEDESC mitschneiden ***/
    memset(t,0,sizeof(struct wdd_TxtFormat));
    memcpy(&(t->ddsd),lpDDSD,sizeof(DDSURFACEDESC));

    /*** folgende Texturformate ignorieren ***/
    if (lpDDSD->ddpfPixelFormat.dwFlags &
        (DDPF_PALETTEINDEXED1    |
         DDPF_PALETTEINDEXED2    |
         DDPF_PALETTEINDEXED4    |
         DDPF_PALETTEINDEXEDTO8))
    {
        return(DDENUMRET_OK);
    };

    if (lpDDSD->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) {
        /*** 8 Bit CLUT! ***/
        t->IsPalettized = TRUE;
        t->IndexBPP     = 8;
        wdd_Log("enum texture formats: 8bpp clut\n");
    } else {
        /*** ein RGB Format! ***/
        unsigned long m;
        int r,g,b,a;
        t->IsPalettized = FALSE;
        t->IndexBPP     = 0;
        if (lpDDSD->ddpfPixelFormat.dwRBitMask) {
            for (r=0, m=lpDDSD->ddpfPixelFormat.dwRBitMask; !(m&1); r++,m>>=1);
            for (r=0; m&1; r++,m>>=1);
        } else r=0;
        if (lpDDSD->ddpfPixelFormat.dwGBitMask) {
            for (g=0, m=lpDDSD->ddpfPixelFormat.dwGBitMask; !(m&1); g++,m>>=1);
            for (g=0; m&1; g++,m>>=1);
        } else g=0;
        if (lpDDSD->ddpfPixelFormat.dwBBitMask) {
            for (b=0, m=lpDDSD->ddpfPixelFormat.dwBBitMask; !(m&1); b++,m>>=1);
            for (b=0; m&1; b++,m>>=1);
        } else b=0;
        if (lpDDSD->ddpfPixelFormat.dwRGBAlphaBitMask) {
            for (a=0, m=lpDDSD->ddpfPixelFormat.dwRGBAlphaBitMask; !(m&1); a++,m>>=1);
            for (a=0; m&1; a++,m>>=1);
        } else a=0;
        t->RedBPP   = r;
        t->GreenBPP = g;
        t->BlueBPP  = b;
        t->AlphaBPP = a;
        t->RGBBPP   = lpDDSD->ddpfPixelFormat.dwRGBBitCount;
        wdd_Log("enum texture formats: %d%d%d%d\n",r,g,b,a);
    };
    wdd->d3d->NumTxtFormats++;
    if (wdd->d3d->NumTxtFormats == WINDD_MAXNUMTXTFORMATS) return(DDENUMRET_CANCEL);
    else                                                   return(DDENUMRET_OK);
}

/*-----------------------------------------------------------------*/
void wdd_Clear(struct windd_data *wdd)
/*
**  FUNCTION
**      Löscht Backbuffer und Z-Buffer in 2D- und 3D-Mode.
**
**  CHANGED
**      08-Mar-97   floh    created
*/
{
    HRESULT ddrval;

    if (wdd_DoDirect3D) {

        /*** 3D-Modus ***/
        unsigned long cflag = D3DCLEAR_TARGET;
        D3DRECT rect;

        if (wdd->d3d->lpDDSZBuffer) cflag |= D3DCLEAR_ZBUFFER;
        rect.x1 = 0;
        rect.y1 = 0;
        rect.x2 = wdd->back_w;
        rect.y2 = wdd->back_h;
        ddrval = wdd->d3d->lpD3DViewport->lpVtbl->Clear(wdd->d3d->lpD3DViewport,
                                                        1, &rect, cflag);
    } else {

        DDBLTFX ddbltfx;

        /*** 2D-Modus ***/
        ddbltfx.dwSize      = sizeof(ddbltfx);
        ddbltfx.dwFillColor = 0;    // FIXME: RASTA_BgPen
        ddrval = wdd->lpDDSBack->lpVtbl->Blt(wdd->lpDDSBack,
                    NULL,   // dest rest
                    NULL,   // src surface
                    NULL,   // src rect
                    DDBLT_COLORFILL|DDBLT_WAIT,
                    &ddbltfx);
    };
}

/*-----------------------------------------------------------------*/
void wdd_DDrawDestroy(void)
/*
**  FUNCTION
**      Globales DirectDraw-Objekt wird abgemeldet,
**      (wenn es korrekt erzeugt wurde).
**      -> die "restlichen" Objekte werden innerhalb der
**         WinProc gekillt, sobald ein WM_DESTROY ankommt!
**
**  CHANGED
**      11-Nov-96   floh    created
**      05-Mar-97   floh    falls initialisiert wird Direct3D Handle
**                          released
*/
{
    if (lpD3D2) {
        lpD3D2->lpVtbl->Release(lpD3D2);
        lpD3D2 = NULL;
    };

    if (lpDD) {
        lpDD->lpVtbl->Release(lpDD);
        lpDD = NULL;
    };
}

/*-----------------------------------------------------------------*/
int wdd_ModeCompare(const void *elm1, const void *elm2)
/*
**  FUNCTION
**      qsort() Hook für das Modes-Array.
**
**  CHANGED
**
**
**  SEE ALSO
*/
{
    struct wdd_DisplayMode *mode1 = (struct wdd_DisplayMode *) elm1;
    struct wdd_DisplayMode *mode2 = (struct wdd_DisplayMode *) elm2;

    if (mode1->bpp < mode2->bpp)      return(-1);
    else if (mode2->bpp < mode1->bpp) return(1);
    else if (mode1->w   < mode2->w)   return(-1);
    else if (mode2->w   < mode1->w)   return(1);
    else if (mode1->h   < mode2->h)   return(-1);
    else if (mode2->h   < mode1->h)   return(1);
    else return(0);
}

/*-----------------------------------------------------------------*/
unsigned long wdd_DDrawCreate()
/*
**  FUNCTION
**      Erzeugt DirectDraw und evtl. Direct3D Object,
**      enumeriert alle verfügbaren 8 Bit
**      Display-Modes.
**
**  INPUTS
**      ---
**
**  RESULTS
**      TRUE    -> alles ok
**      FALSE   -> hat nichts geklappt
**
**  CHANGED
**      11-Nov-96   floh    created
**      22-Nov-96   floh    BUG: Oh Ghott...
**      26-Jan-97   floh    + bei Windowed-Modes werden 2 Versionen
**                            angeboten, mit Backbuffer im SysMem oder
**                            VMem
**      03-Mar-97   floh    + wertet jetzt aus:
**                              wdd_DoDirect3D
**      23-Mar-97   floh    + CanDoWindow wurde falsch initialisiert
**                            bei Treibern die in mehr als 1 Bittiefe
**                            rendern können
**      03-Mar-98   floh    + wdd_Log()
**      04-Mar-98   floh    + 3D-Device wird jetzt komplett anders
**                            ermittelt... (nicht per FindDevice,
**                            sondern EnumDevices innerhalb
**                            DDrawEnumerate. FindDevice scheint eine
**                            Macke zu haben.
**      10-Mar-98   floh    + es werden jetzt alle D3D-Devices enumeriert,
**                            und es wird vorerst (als Hack) immer
**                            das LETZTE gefundene Device benutzt
**      11-Mar-98   floh    + Device-Picking jetzt mit Hilfe des
**                            externen Files guid3d.def.
**      18-Mar-98   floh    + Bei AddDisplayMode wird der zur Verfügung
**                            Display-Mem mit beachtet.
*/
{
    HRESULT ddrval;
    DDSURFACEDESC ddsd;
    unsigned long i;
    DDCAPS driver_caps, hel_caps;

    /*** Global Data initialisieren ***/
    lpDD  = NULL;
    lpD3D2 = NULL;
    memset(&wdd_Data,0,sizeof(wdd_Data));
    wdd_Log("dd/d3d init: entered wdd_DDrawCreate()\n");

    /*** alle 3D-Devices enumerieren (auch im DDraw-Modus) ***/
    ddrval = DirectDrawEnumerate((LPDDENUMCALLBACK)wdd_DDrawEnumCallback,NULL);
    if (ddrval != DD_OK) {
        wdd_FailMsg("DirectDraw","DirectDrawEnumerate()",ddrval);
        wdd_Log("common init failed: DirectDrawEnumerate()\n");
        return(FALSE);
    };

    /*** DirectDraw Interface holen (jetzt etwas komplexer) ***/
    if (wdd_DoDirect3D) {

        GUID wanted_guid;
        unsigned long wanted_guid_status;

        /*** lese gewollte GUID ein ***/
        wanted_guid_status = wdd_ReadGUID("env/guid3d.def",&wanted_guid);
        switch(wanted_guid_status) {
            char guid_buf[128];
            case 0:
                wdd_Log("d3d init: guid3d.def invalid\n");
                break;
            case 1:
                wdd_Guid2Str(guid_buf,&wanted_guid,"<error>");
                wdd_Log("d3d init: guid3d.def is %s\n",guid_buf);
                break;
            case 2:
                wdd_Log("d3d init: guid3d.def is <primary>\n");
                break;
            case 3:
                wdd_Log("d3d init: guid3d.def is <software>\n");
                break;
        };

        /*** erzeuge DDraw und D3D Object ***/
        if (wdd_Data.NumDrivers > 0) {

            struct wdd_3DDriver *d;
            GUID FAR *lpDDrawGUID;
            unsigned long guid_match_found = FALSE;

            if (wanted_guid_status == 3) {
                /*** es wird der Software-Renderer verlangt... also raus ***/
                wdd_Log("d3d init: ddraw mode wanted, exit\n");
                wdd_DDrawDestroy();
                return(FALSE);
            } else if ((wanted_guid_status==2) || (wanted_guid_status==1)) {
                /*** es wird das Primary, oder eine GUID verlangt... ***/
                for (i=0; i<wdd_Data.NumDrivers; i++) {
                    d = &(wdd_Data.AllDrivers[i]);
                    if (wanted_guid_status==2) {
                        /*** es wird das Primary verlangt ***/
                        if (!(d->DDrawGuidIsValid)) {
                            /*** Treffer! ***/
                            wdd_Log("d3d init: found match for guid3d.def\n");
                            guid_match_found = TRUE;
                            wdd_Data.ActDriver = i;
                            break;
                        };
                    } else if (wanted_guid_status==3) {
                        /*** es wird nach einer GUID gesucht ***/
                        if (d->DDrawGuidIsValid) {
                            if (memcmp(&wanted_guid,&(d->DDrawGuid),sizeof(GUID)) == 0) {
                                /*** hier haben wir eine Übereinstimmung ***/
                                wdd_Log("d3d init: found match for guid3d.def\n");
                                guid_match_found = TRUE;
                                wdd_Data.ActDriver = i;
                                break;
                            };
                        };
                    };
                };
            };

            /*** was gefunden? ***/
            if (!guid_match_found) {
                wdd_Log("d3d init: no guid3d.def match found, using autodetect\n");
                wdd_Data.ActDriver = wdd_Data.NumDrivers-1;
            };

            /*** wdd_Data.ActDriver ist jetzt gültig ***/
            memcpy(&(wdd_Data.Driver),&(wdd_Data.AllDrivers[wdd_Data.ActDriver]),sizeof(struct wdd_3DDriver));
            d = &(wdd_Data.Driver);
            if (d->DDrawGuidIsValid) lpDDrawGUID = &(d->DDrawGuid);
            else                     lpDDrawGUID = NULL;
            wdd_Log("picked: %s, %s\n",wdd_Data.Driver.DeviceName,wdd_Data.Driver.DeviceDesc);
            wdd_WriteGUID("env/guid3d.def",lpDDrawGUID,"<primary>");
            ddrval = DirectDrawCreate(lpDDrawGUID,&lpDD,NULL);
            if (ddrval == DD_OK) {
                ddrval = lpDD->lpVtbl->QueryInterface(lpDD,&IID_IDirect3D2,&lpD3D2);
            };
        };

        /*** jetzt müßte lpDD und lpD3D2 initialisiert sein ***/
        if (lpD3D2) {
            /*** iss dat ein Fullscreen-Karte? ***/
            if (wdd_Data.Driver.DDrawGuidIsValid) {
                wdd_Data.Flags &= ~WINDDF_CanDoWindowed;
                wdd_Data.Driver.CanDoWindow  = FALSE;
                wdd_Data.Driver.DoSoftCursor = TRUE;
                wdd_Log("d3d init: non-primary device picked, assuming fullscreen card\n");
            };
        } else {
            /*** keine geeigneten Devices gefunden ***/
            wdd_Log("d3d init failed: no suitable d3d device found.\n");
            wdd_DDrawDestroy();
            return(FALSE);
        };
    };

    if (!lpDD) {
        /*** DDraw-Mode benutzt einfach das primary DDraw Object ***/
        ddrval = DirectDrawCreate(NULL,&lpDD,NULL);
        if (ddrval != DD_OK) {
            wdd_FailMsg("DirectDraw","DirectDrawCreate()",ddrval);
            wdd_Log("common init failed: DirectDrawCreate()\n");
            return(FALSE);
        };
    };

    /*** wieviel VidMem ham wir denn? ***/
    memset(&driver_caps,0,sizeof(driver_caps));
    driver_caps.dwSize = sizeof(driver_caps);
    memset(&hel_caps,0,sizeof(driver_caps));
    hel_caps.dwSize = sizeof(driver_caps);
    if (lpDD->lpVtbl->GetCaps(lpDD,&driver_caps,&hel_caps) == DD_OK) {
        wdd_Data.VidMemTotal = driver_caps.dwVidMemTotal;
    } else {
        wdd_Data.VidMemTotal = 0;
    };
    wdd_Log("common init: vmem total is %d\n", wdd_Data.VidMemTotal);

    /*** *alle* DisplayModes enumerieren ***/
    ddrval = lpDD->lpVtbl->EnumDisplayModes(lpDD,0,NULL,NULL,wdd_EnumDisplayModesCallback);
    if (ddrval != DD_OK) {
        wdd_FailMsg("DirectDraw","DirectDraw::EnumDisplayModes()",ddrval);
        wdd_Log("common init failed: EnumDisplayModes()\n");
        return(FALSE);
    };

    /*** 3D-Devices exportieren ***/
    wdd_AddDevice("software","<software>",wdd_DoDirect3D ? FALSE:TRUE);
    for (i=0; i<wdd_Data.NumDrivers; i++) {
        struct wdd_3DDriver *d = &(wdd_Data.AllDrivers[i]);
        unsigned long is_current = ((i == wdd_Data.ActDriver) && wdd_DoDirect3D);
        if (!(d->DDrawGuidIsValid)) {
            /*** das ist das Primary Device ***/
            wdd_AddDevice(d->DDrawDesc, "<primary>", is_current);
        } else {
            /*** es ist ein "anderes" Device ***/
            char buf[256];
            wdd_Guid2Str(buf,&(d->DDrawGuid),"<error>");
            wdd_AddDevice(d->DDrawDesc, buf, is_current);
        };
    };

    /*** Display-Modes sortieren und gefiltert exportieren ***/
    qsort((void *)&(wdd_Data.Modes[0]), (size_t)wdd_Data.NumModes,
          sizeof(struct wdd_DisplayMode), wdd_ModeCompare);
    for (i=0; i<wdd_Data.NumModes; i++) {

        struct wdd_DisplayMode *mode = &(wdd_Data.Modes[i]);

        if (wdd_DoDirect3D) {

            /*** 3D-Modus: nur 16 Bit Modes, in die das Device rendern kann ***/
            struct wdd_3DDriver *d = &(wdd_Data.Driver);
            unsigned long depths = d->Desc.dwDeviceRenderBitDepth;
            if ((wdd_BPP2DDBD(mode->bpp) & depths) && (mode->bpp == 16))
            {
                /*** der ist ok ***/
                mode->can_do_3d = TRUE; // nur der Ordnung halber
                wdd_AddDisplayMode(mode->w,mode->h,mode->bpp,WINDDF_IsDirect3D);
                wdd_Log("d3d init: export display mode %dx%dx%d\n",mode->w,mode->h,mode->bpp);
            };

        }else{

            /*** 2D-Modus: alles, was 8 Bit ist ***/
            if (mode->bpp == 8)
            {
                /*** Fullscreen-Half-Modes ***/
                if (mode->w >= 640) {
                    wdd_AddDisplayMode(mode->w,mode->h,mode->bpp,WINDDF_IsFullScrHalf);
                };
                /*** und der normale Modus ***/
                wdd_AddDisplayMode(mode->w,mode->h,mode->bpp,0);
                wdd_Log("dd init: export display mode %dx%dx%d\n",mode->w,mode->h,mode->bpp);
            };
        };
    };
    /*** noch ein paar Windowed-Modes dazu ***/
    if (wdd_DoDirect3D) {
        if (wdd_Data.Driver.CanDoWindow) {
            wdd_AddDisplayMode(320,200,wdd_Data.Desktop.ddpfPixelFormat.dwRGBBitCount,
                               WINDDF_IsWindowed|WINDDF_IsDirect3D);
            wdd_Log("dd init: export windowed mode %dx%dx%d\n",320,200,wdd_Data.Desktop.ddpfPixelFormat.dwRGBBitCount);

        };
    } else {
        if (wdd_Data.Desktop.ddpfPixelFormat.dwRGBBitCount == 8) {
            wdd_AddDisplayMode(320,200,8,WINDDF_IsWindowed|WINDDF_IsSysMem);
            wdd_Log("dd init: export windowed mode %dx%dx%d\n",320,200,wdd_Data.Desktop.ddpfPixelFormat.dwRGBBitCount);
        };
    };

    /*** Der Rest wird in wdd_InitDDDrawStuff erledigt ***/
    wdd_Log("dd/d3d init: wdd_DDrawCreate() left\n");
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void wdd_KillDDrawObjects(struct windd_data *wdd)
/*
**  FUNCTION
**      Die DirectDraw-Objects müssen auf alle
**      Fälle gekillt werden, BEVOR das Window-
**      Handle ungültig wird. Damit dies in
**      jedem denkbaren Fall sichergestellt ist,
**      wird dies direkt aus der wdd_WinProc()
**      erledigt.
**
**  CHANGED
**      11-Nov-96   floh    created
**      06-Mar-97   floh    ruft wdd_Kill3D() auf
*/
{
    /*** Direct3D-specifics killen ***/
    if (wdd->d3d) wdd_Kill3D(wdd);

    /*** Palette killen ***/
    if (wdd->lpDDPal) {
        wdd->lpDDPal->lpVtbl->Release(wdd->lpDDPal);
        wdd->lpDDPal = NULL;
    };

    /*** Clipper killen ***/
    if (wdd->lpDDClipper) {
        wdd->lpDDClipper->lpVtbl->Release(wdd->lpDDClipper);
        wdd->lpDDClipper = NULL;
    };

    /*** Back Surface killen ***/
    if (wdd->lpDDSBack) {
        wdd->lpDDSBack->lpVtbl->Release(wdd->lpDDSBack);
        wdd->lpDDSBack = NULL;
    };

    /*** Primary Surface killen ***/
    if (wdd->lpDDSPrim) {
        wdd->lpDDSPrim->lpVtbl->Release(wdd->lpDDSPrim);
        wdd->lpDDSPrim = NULL;
    };

    /*** Backup-Surface, falls existent ***/
    if (wdd->lpDDSBackup) {
        wdd->lpDDSBackup->lpVtbl->Release(wdd->lpDDSBackup);
        wdd->lpDDSBackup = NULL;
    };
}

/*-----------------------------------------------------------------*/
unsigned long wdd_ValidateWindow(struct windd_data *wdd,
                                 HINSTANCE inst,
                                 int cmd_show,
                                 unsigned long w,
                                 unsigned long h)
/*
**  FUNCTION
**      Erzeugt oder (falls schon existent) modifiziert Window
**      entsprechend Höhe/Breite/Flags...
**
**  INPUTS
**      wdd      - LID des windd.class Objects
**      inst     - HINSTANCE Handle der Applikation (win_main.c/win_Instance)
**      cmd_show - CmdShow der App (win_main.c/win_CmdShow)
**      w,h      - Höhe und Breite, wenn Windowed
**
**  RESULTS
**      > validiert win_HWND
**      TRUE:   all ok
**      FALSE:  Fehler
**
**  CHANGED
**      14-Feb-97   floh    created
**      21-Apr-97   floh    Fixes für eigenartiges Monster3d Verhalten
**                          (Displaymode-Größe != Desktop-Größe)
*/
{
    HICON   hIcon   = NULL;
    HCURSOR hCursor = NULL;
    WNDCLASS      wc;

    /*** Resourcen sicherstellen ***/
    hIcon   = LoadIcon(inst,"Big256");
    hCursor = LoadCursor(inst,"Pointer");

    /*** Window global nur 1x öffnen! ***/
    if (!win_HWnd) {

        /*** Window Class anmelden ***/
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wc.lpfnWndProc = wdd_WinProc;
        wc.cbClsExtra  = 4;     // Größe der User-Data
        wc.cbWndExtra  = 0;
        wc.hInstance   = inst;
        wc.hIcon       = hIcon;
        wc.hCursor     = NULL;
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wc.lpszMenuName  = NULL;
        wc.lpszClassName = WDD_WINDOW_CLASS;
        RegisterClass(&wc);

        /*** Window erzeugen ***/
        if (wdd->flags & WINDDF_IsWindowed) {
            wdd->hWnd = CreateWindowEx(
                        0,
                        WDD_WINDOW_CLASS,
                        "Urban Assault",
                        WS_OVERLAPPEDWINDOW,
                        0, 0,
                        w, h,
                        NULL, NULL,
                        inst, NULL);
        } else {
            wdd->hWnd = CreateWindowEx(
                        WS_EX_TOPMOST,
                        WDD_WINDOW_CLASS,
                        "Urban Assault",
                        WS_POPUP|WS_SYSMENU,
                        0, 0,
                        GetSystemMetrics(SM_CXSCREEN),
                        GetSystemMetrics(SM_CYSCREEN),
                        NULL, NULL,
                        inst, NULL);
        };
        if (NULL == wdd->hWnd) return(FALSE);

        /*** evtl IME abschalten und Window anzeigen ***/
        win_HImc = ImmAssociateContext(wdd->hWnd,NULL);
        ShowWindow(wdd->hWnd,cmd_show);
        UpdateWindow(wdd->hWnd);
        SetCursor(hCursor);

        /*** globalen Pointer setzen ***/
        win_HWnd = wdd->hWnd;

    } else {

        /*** Window existierte bereits ***/
        wdd->hWnd = win_HWnd;
        if (wdd->flags & WINDDF_IsWindowed) {
            /*** Windowed: Style, Ausdehnung neu setzen ***/
            SetWindowLong(wdd->hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
            SetWindowPos(wdd->hWnd, 0, 0, 0, w, h, SWP_SHOWWINDOW);
        } else {
            /*** Fullscreen: Style, Position und Ausdehnung neu setzen ***/
            SetWindowLong(wdd->hWnd, GWL_STYLE, WS_POPUP|WS_SYSMENU);
            SetWindowLong(wdd->hWnd, GWL_EXSTYLE, WS_EX_TOPMOST);
            SetWindowPos(wdd->hWnd, 0, 0, 0,
                         GetSystemMetrics(SM_CXSCREEN),
                         GetSystemMetrics(SM_CYSCREEN),
                         SWP_SHOWWINDOW);
        };
    };

    /*** UserData in Window-Instanz schreiben ***/
    SetClassLong(wdd->hWnd,0,(LONG)wdd);

    /*** alles ok ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
unsigned long wdd_Create2DWinEnv(struct windd_data *wdd,
                                 PALETTEENTRY *pal,
                                 unsigned long w,
                                 unsigned long h)
/*
**  FUNCTION
**      Erzeugt die DirectDraw-Umgebung für den 2D-Windowed-Modus.
**
**  RESULTS
**      wdd->lpDDSPrim
**      wdd->lpDDSBack
**      wdd->lpDDSClipper
**      wdd->lpDDSPalette
**
**      TRUE    -> alles OK
**      FALSE   -> etwas ging schief
**
**  CHANGED
**      14-Feb-96   floh    created
**      26-Nov-97   floh    Fehlerbehandlung fuer SetCooperativeLevel()
**                          rausgenommen.
**      07-Jan-98   floh    Palette wird auch an BackSurface attached
*/
{
    /*** WINDOWED MODE (Debugging und 8 bpp ONLY!) ***/
    HRESULT       ddrval;
    DDSURFACEDESC ddsd;
    DDSCAPS       ddscaps;

    wdd_Log("-> Entering wdd_Create2DWinEnv()\n");

    /*** wir sind kooperativ... ***/
    wdd->flags |= WINDDF_Is8BppCLUT;
    ddrval = lpDD->lpVtbl->SetCooperativeLevel(lpDD,wdd->hWnd,DDSCL_NORMAL);

    /*** Primary-Surface erzeugen ***/
    memset(&ddsd,0,sizeof(ddsd));
    ddsd.dwSize  = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    ddrval = lpDD->lpVtbl->CreateSurface(lpDD,&ddsd,&(wdd->lpDDSPrim),NULL);
    if (ddrval != DD_OK) {
        wdd_FailMsg("windd.class","DirectDraw::CreateSurface(Primary)",ddrval);
        return(FALSE);
    };

    /*** Back-Surface erzeugen (immer im System-Memory) ***/
    memset(&ddsd,0,sizeof(ddsd));
    ddsd.dwSize  = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN|DDSCAPS_SYSTEMMEMORY;
    ddsd.dwHeight = h;
    ddsd.dwWidth  = w;
    ddrval = lpDD->lpVtbl->CreateSurface(lpDD,&ddsd,&(wdd->lpDDSBack),NULL);
    if (ddrval != DD_OK) {
        wdd_FailMsg("windd.class","DirectDraw::CreateSurface(Back)",ddrval);
        return(FALSE);
    };

    /*** Clipper Objekt erzeugen ***/
    ddrval = lpDD->lpVtbl->CreateClipper(lpDD,0,&(wdd->lpDDClipper),NULL);
    if (ddrval != DD_OK) {
        wdd_FailMsg("windd.class","DirectDraw::CreateClipper()",ddrval);
        return(FALSE);
    };
    wdd->lpDDClipper->lpVtbl->SetHWnd(wdd->lpDDClipper,0,wdd->hWnd);
    wdd->lpDDSPrim->lpVtbl->SetClipper(wdd->lpDDSPrim,wdd->lpDDClipper);

    /*** Paletten-Object erzeugen und attachen ***/
    ddrval = lpDD->lpVtbl->CreatePalette(lpDD,DDPCAPS_8BIT,pal,&(wdd->lpDDPal),NULL);
    if (ddrval != DD_OK) {
        wdd_FailMsg("windd.class","DirectDraw::CreatePalette()",ddrval);
        return(FALSE);
    };
    ddrval = wdd->lpDDSPrim->lpVtbl->SetPalette(wdd->lpDDSPrim,wdd->lpDDPal);
    ddrval = wdd->lpDDSBack->lpVtbl->SetPalette(wdd->lpDDSBack,wdd->lpDDPal);

    wdd_Log("-> Leaving wdd_Create2DWinEnv()\n");
    return(TRUE);
}

/*-----------------------------------------------------------------*/
unsigned long wdd_Create2DFullEnv(struct windd_data *wdd,
                                  PALETTEENTRY *pal,
                                  unsigned long w,
                                  unsigned long h)
/*
**  FUNCTION
**      Erzeugt die DirectDraw-Umgebung für den 2D-Fullscreen-Modus.
**
**  RESULTS
**      wdd->lpDDSPrim
**      wdd->lpDDSBack
**      wdd->lpDDSPalette
**
**      TRUE    -> alles OK
**      FALSE   -> etwas ging schief
**
**  CHANGED
**      14-Feb-96   floh    created
**      05-Mar-97   floh    oops, kein Rückgabewert bei Erfolg
**      17-Aug-97   floh    erzeugt jetzt auch ein Clipper-Object
**      26-Nov-97   floh    + kein Abbruch mehr, wenn SetCooperativeLevel
**                            mit einem Fehler zurueckkomt.
**      07-Jan-98   floh    + Palette wird auch an BackSurface attached
*/
{
    /*** 2D FULLSCREEN MODE ***/
    HRESULT       ddrval;
    DDSURFACEDESC ddsd;
    DDSCAPS       ddscaps;
    
    wdd_Log("-> Entering wdd_Create2DFullEnv()\n");    

    /*** immer 8bpp ***/
    wdd->flags |= WINDDF_Is8BppCLUT;

    /*** Exclusive Fullscreen ***/
    ddrval = lpDD->lpVtbl->SetCooperativeLevel(lpDD,wdd->hWnd,DDSCL_EXCLUSIVE|DDSCL_FULLSCREEN);
    ddrval = lpDD->lpVtbl->SetDisplayMode(lpDD,w,h,8);
    if (ddrval != DD_OK) {
        wdd_FailMsg("windd.class","DirectDraw::SetDisplayMode()",ddrval);
        return(FALSE);
    };

    /*** Prim-Surface erzeugen ***/
    memset(&ddsd,0,sizeof(ddsd));
    ddsd.dwSize  = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    ddrval = lpDD->lpVtbl->CreateSurface(lpDD,&ddsd,&(wdd->lpDDSPrim),NULL);
    if (ddrval != DD_OK) {
        wdd_FailMsg("windd.class","DirectDraw::CreateSurface(Primary)",ddrval);
        return(FALSE);
    };

    /*** Backbuffer immer im Sysmem! ***/
    ddsd.dwFlags = DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN|DDSCAPS_SYSTEMMEMORY;
    if (wdd->flags & WINDDF_IsFullScrHalf) {
        ddsd.dwHeight = h>>1;
        ddsd.dwWidth  = w>>1;
    } else {
        ddsd.dwHeight = h;
        ddsd.dwWidth  = w;
    };
    ddrval = lpDD->lpVtbl->CreateSurface(lpDD,&ddsd,&(wdd->lpDDSBack),NULL);
    if (ddrval != DD_OK) {
        wdd_FailMsg("windd.class","DirectDraw::CreateSurface(Back)",ddrval);
        return(FALSE);
    };

    /*** Clipper Objekt erzeugen ***/
    ddrval = lpDD->lpVtbl->CreateClipper(lpDD,0,&(wdd->lpDDClipper),NULL);
    if (ddrval != DD_OK) {
        wdd_FailMsg("windd.class","DirectDraw::CreateClipper()",ddrval);
        return(FALSE);
    };
    wdd->lpDDClipper->lpVtbl->SetHWnd(wdd->lpDDClipper,0,wdd->hWnd);
    wdd->lpDDSPrim->lpVtbl->SetClipper(wdd->lpDDSPrim,wdd->lpDDClipper);

    /*** Paletten-Object erzeugen und attachen ***/
    ddrval = lpDD->lpVtbl->CreatePalette(lpDD,DDPCAPS_8BIT|DDPCAPS_ALLOW256,pal,&(wdd->lpDDPal),NULL);
    if (ddrval != DD_OK) {
        wdd_FailMsg("windd.class","CreatePalette()",ddrval);
        return(FALSE);
    };
    ddrval = wdd->lpDDSPrim->lpVtbl->SetPalette(wdd->lpDDSPrim,wdd->lpDDPal);
    ddrval = wdd->lpDDSBack->lpVtbl->SetPalette(wdd->lpDDSBack,wdd->lpDDPal);

    wdd_Log("-> Leaving wdd_Create2DFullEnv()\n");
    return(TRUE);
}

/*-----------------------------------------------------------------*/
unsigned long wdd_Create3DWinEnv(struct windd_data *wdd,
                                 unsigned long w,
                                 unsigned long h,
                                 unsigned long bpp)
/*
**  FUNCTION
**      Initialisiert 3D-Windowed-Environment. Der Desktop
**      muß in einer kompatiblen Bittiefe eingestellt sein,
**      was aber durch den Aufbau der Display-Mode-Liste
**      in wdd_InitDDrawStuff() sichergestellt wurde.
**
**      Initialisiert wird:
**          wdd->lpDDSPrim
**          wdd->lpDDSBack
**          wdd->lpDDSClipper
**
**  CHANGED
**      06-Mar-97   floh    created
**      26-Nov-97   floh    + kein Abbruch mehr, wenn SetCooperativeLevel()
**                            mit einem Fehler zurueckkommt.
*/
{
    /*** 3D WINDOWED MODE ***/
    HRESULT       ddrval;
    DDSURFACEDESC ddsd;
    DDSCAPS       ddscaps;
    DDBLTFX ddbltfx;
    
    wdd_Log("-> Entering wdd_Create3DWinEnv()\n");    

    /*** kooperationsbereit... ***/
    ddrval = lpDD->lpVtbl->SetCooperativeLevel(lpDD,wdd->hWnd,DDSCL_NORMAL);

    /*** benötigte Buffer erzeugen (wir sind in Windowed Mode) ***/
    /*** also ein Front-Buffer (Primary Surface) und ein Back- ***/
    /*** Buffer, der aber im Videomem liegt!                   ***/
    memset(&ddsd,0,sizeof(DDSURFACEDESC));
    ddsd.dwSize  = sizeof(DDSURFACEDESC);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    ddrval = lpDD->lpVtbl->CreateSurface(lpDD,&ddsd,&(wdd->lpDDSPrim),NULL);
    if (ddrval != DD_OK) {
        wdd_FailMsg("windd.class","DirectDraw::CreateSurface(Primary)",ddrval);
        return(FALSE);
    };

    ddsd.dwFlags = DDSD_WIDTH|DDSD_HEIGHT|DDSD_CAPS;
    ddsd.dwWidth  = w;
    ddsd.dwHeight = h;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN|
                          DDSCAPS_3DDEVICE|
                          DDSCAPS_VIDEOMEMORY;
    ddrval = lpDD->lpVtbl->CreateSurface(lpDD,&ddsd,&(wdd->lpDDSBack),NULL);
    if (ddrval != DD_OK) {
        wdd_FailMsg("windd.class","DirectDraw::CreateSurface(Back)",ddrval);
        return(FALSE);
    };

    /*** Clipper-Object erzeugen ***/
    ddrval = lpDD->lpVtbl->CreateClipper(lpDD,0,&(wdd->lpDDClipper),NULL);
    if (ddrval != DD_OK) {
        wdd_FailMsg("windd.class","DirectDraw::CreateClipper()",ddrval);
        return(FALSE);
    };
    wdd->lpDDClipper->lpVtbl->SetHWnd(wdd->lpDDClipper,0,wdd->hWnd);
    wdd->lpDDSPrim->lpVtbl->SetClipper(wdd->lpDDSPrim,wdd->lpDDClipper);

    /*** loesche Primary- und Back-Surface ***/
    ddbltfx.dwSize      = sizeof(ddbltfx);
    ddbltfx.dwFillColor = 0;
    ddrval = wdd->lpDDSPrim->lpVtbl->Blt(wdd->lpDDSPrim,
                NULL, NULL, NULL, DDBLT_COLORFILL|DDBLT_WAIT,
                &ddbltfx);
    ddrval = wdd->lpDDSBack->lpVtbl->Blt(wdd->lpDDSBack,
                NULL, NULL, NULL, DDBLT_COLORFILL|DDBLT_WAIT,
                &ddbltfx);
                
    wdd_Log("-> Leaving wdd_Create3DWinEnv()\n");                
    return(TRUE);
}

/*-----------------------------------------------------------------*/
unsigned long wdd_Create3DFullEnv(struct windd_data *wdd,
                                 unsigned long w,
                                 unsigned long h,
                                 unsigned long bpp)
/*
**  FUNCTION
**      Initialisiert 3D-Fullscreen-Environment, das 3D-Device
**      muß in der Lage sein, in die ausgewählte Bittiefe
**      zu rendern (sichergestellt durch wdd_DDrawCreate().
**          wdd->lpDDSPrim
**          wdd->lpDDSBack
**          wdd->lpDDSClipper
**
**  CHANGED
**      06-Mar-97   floh    created
**      21-Apr-97   floh    + Monster3D-Fixes
**      17-Aug-97   floh    + erzeugt jetzt auch ein Clipper-Object
**      26-Nov-97   floh    + kein Abbruch mehr, wenn SetCooperativeLevel()
**                            mit einem Abbruch zurueckkommt.
**      18-Dec-97   floh    + Primary Surface wird jetzt geloescht.
*/
{
    HRESULT       ddrval;
    DDSURFACEDESC ddsd;
    DDSCAPS       ddscaps;
    RECT r;
    DDBLTFX ddbltfx;
    
    wdd_Log("-> Entering wdd_Create3DFullEnv()\n");    

    /*** Fullscreen, also Exclusive Mode ***/
    ddrval = lpDD->lpVtbl->SetCooperativeLevel(lpDD,wdd->hWnd,DDSCL_EXCLUSIVE|DDSCL_FULLSCREEN);

    /*** DisplayMode setzen ***/
    ddrval = lpDD->lpVtbl->SetDisplayMode(lpDD,w,h,bpp);
    if (ddrval != DD_OK) {
        wdd_FailMsg("windd.class","DirectDraw::SetDisplayMode()",ddrval);
        return(FALSE);
    };

    /*** Erzeuge eine "Complex Flipping Surface" für Fullscreen- ***/
    /*** Modus mit einem Backbuffer                              ***/
    memset(&ddsd,0,sizeof(DDSURFACEDESC));
    ddsd.dwSize  = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE |
                          DDSCAPS_FLIP |
                          DDSCAPS_3DDEVICE |
                          DDSCAPS_COMPLEX |
                          DDSCAPS_VIDEOMEMORY;
    ddsd.dwBackBufferCount = 1;
    ddrval = lpDD->lpVtbl->CreateSurface(lpDD,&ddsd,&(wdd->lpDDSPrim),NULL);
    if (ddrval != DD_OK) {
        wdd_FailMsg("windd.class","DirectDraw::CreateSurface(Primary)",ddrval);
        return(FALSE);
    };

    ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
    ddrval = wdd->lpDDSPrim->lpVtbl->GetAttachedSurface(wdd->lpDDSPrim,
                                     &ddscaps,&(wdd->lpDDSBack));
    if (ddrval != DD_OK) {
        wdd_FailMsg("windd.class","DirectDraw::GetAttachedSurface(Back)",ddrval);
        return(FALSE);
    };

    /*** Clipper-Object erzeugen ***/
    ddrval = lpDD->lpVtbl->CreateClipper(lpDD,0,&(wdd->lpDDClipper),NULL);
    if (ddrval != DD_OK) {
        wdd_FailMsg("windd.class","DirectDraw::CreateClipper()",ddrval);
        return(FALSE);
    };
    wdd->lpDDClipper->lpVtbl->SetHWnd(wdd->lpDDClipper,0,wdd->hWnd);
    wdd->lpDDSPrim->lpVtbl->SetClipper(wdd->lpDDSPrim,wdd->lpDDClipper);

    /*** loesche Primary- und Back-Surface ***/
    ddbltfx.dwSize      = sizeof(ddbltfx);
    ddbltfx.dwFillColor = 0;
    ddrval = wdd->lpDDSPrim->lpVtbl->Blt(wdd->lpDDSPrim,
                NULL, NULL, NULL, DDBLT_COLORFILL|DDBLT_WAIT,
                &ddbltfx);
    ddrval = wdd->lpDDSBack->lpVtbl->Blt(wdd->lpDDSBack,
                NULL, NULL, NULL, DDBLT_COLORFILL|DDBLT_WAIT,
                &ddbltfx);
    wdd_Log("-> Leaving wdd_Create3DFullEnv()\n");
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void wdd_Kill3D(struct windd_data *wdd)
/*
**  FUNCTION
**      Gegenstück zu wdd_Create3D().
**
**  CHANGED
**      05-Mar-97   floh    created
**      10-May-98   floh    zusaetzlich lpD3DDevice2 und lpD3DViewport2
*/
{
    wdd_Log("-> Entering wdd_Kill3D()\n");
    if (wdd->d3d) {

        /*** alle D3D-spezifischen Objekte killen ***/
        if (wdd->d3d->lpBackMat) {
            wdd->d3d->lpBackMat->lpVtbl->Release(wdd->d3d->lpBackMat);
            wdd->d3d->lpBackMat = NULL;
        };
        if (wdd->d3d->lpD3DExecBuffer) {
            wdd->d3d->lpD3DExecBuffer->lpVtbl->Release(wdd->d3d->lpD3DExecBuffer);
            wdd->d3d->lpD3DExecBuffer = NULL;
        };
        if (wdd->d3d->lpD3DViewport) {
            wdd->d3d->lpD3DViewport->lpVtbl->Release(wdd->d3d->lpD3DViewport);
            wdd->d3d->lpD3DViewport = NULL;
        };
        if (wdd->d3d->lpD3DDevice) {
            wdd->d3d->lpD3DDevice->lpVtbl->Release(wdd->d3d->lpD3DDevice);
            wdd->d3d->lpD3DDevice = NULL;
        };
        if (wdd->d3d->lpD3DDevice2) {
            wdd->d3d->lpD3DDevice2->lpVtbl->Release(wdd->d3d->lpD3DDevice2);
            wdd->d3d->lpD3DDevice2 = NULL;
        };
        if (wdd->d3d->lpDDSZBuffer) {
            wdd->d3d->lpDDSZBuffer->lpVtbl->Release(wdd->d3d->lpDDSZBuffer);
            wdd->d3d->lpDDSZBuffer = NULL;
        };

        /*** Direct3D-Parameter-Struktur killen ***/
        free(wdd->d3d);
        wdd->d3d = NULL;
    };
    wdd_Log("-> Leaving wdd_Kill3D()\n");
}

/*-----------------------------------------------------------------*/
unsigned long wdd_Create3D(struct windd_data *wdd,
                           unsigned long w,
                           unsigned long h,
                           unsigned long bpp)
/*
**  FUNCTION
**      Initialisierung der allgemeinen Direct3D-Parameter,
**      unabhängig von Windowed oder Fullscreen.
**
**  CHANGED
**      05-Mar-97   floh    created
**      20-Aug-97   floh    + Textureformat-Auswahl verändert,
**                            zuerst 8-bpp-CLUT, dann 16-bpp-4444,
**                            dann 32-bpp-8888
**      21-Aug-97   floh    + 8-Bit-Texturen werden nur noch akzeptiert,
**                            wenn auch Colorkeying funktioniert.
**      03-Mar-98   floh    + updated
**                          + nimmt, falls vorhanden, einen 16-Bit-ZBuffer
**                            (vorher konnte er einen 8-Bit erwischen)
*/
{
    HRESULT d3dval,ddrval;
    unsigned long i;
    long txt_8bpp_fit,txt_16bpp_fit,txt_32bpp_fit;
    unsigned long dsp_r_mask,dsp_g_mask,dsp_b_mask;
    DDSURFACEDESC ddsd;
    D3DVIEWPORT vport;
    D3DMATERIAL bmat,omat;
    D3DMATERIALHANDLE hbmat,homat;
    unsigned long exbuf_size;
    D3DEXECUTEBUFFERDESC exbuf_desc;

    wdd_Log("-> Entering wdd_Create3D()\n");

    /*** allokiere Pro-Objekt-Direct3D-Parameter-Struktur ***/
    wdd->d3d = malloc(sizeof(struct wdd_D3D));
    if (wdd->d3d) memset(wdd->d3d,0,sizeof(struct wdd_D3D));
    else {
        wdd_Kill3D(wdd);
        return(FALSE);
    };

    /*** initialisiere Z-Buffer ***/
    if (wdd_Data.Driver.DoesZBuffer) {

        unsigned long depth;

        memset(&ddsd,0,sizeof(ddsd));
        ddsd.dwSize  = sizeof(ddsd);
        ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS |
                       DDSD_ZBUFFERBITDEPTH;
        ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
        ddsd.dwWidth  = w;
        ddsd.dwHeight = h;
        ddsd.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
        ddsd.dwZBufferBitDepth = wdd_Data.Driver.ZBufBitDepth;
        wdd_Log("d3d init: create zbuf (w=%d,h=%d,bpp=%d)\n",ddsd.dwWidth,ddsd.dwHeight,ddsd.dwZBufferBitDepth);
        ddrval = lpDD->lpVtbl->CreateSurface(lpDD,&ddsd,&(wdd->d3d->lpDDSZBuffer),NULL);
        if (ddrval != DD_OK) {
            wdd_FailMsg("DirectDraw","Could not create z buffer.",ddrval);
            wdd_Log("d3d init: zbuf creation failed\n");
            wdd_Kill3D(wdd);
            return(FALSE);
        };

        /*** ZBuffer an Backbuffer attachen ***/
        ddrval = wdd->lpDDSBack->lpVtbl->AddAttachedSurface(wdd->lpDDSBack,
                                         wdd->d3d->lpDDSZBuffer);
        if (ddrval != DD_OK) {
            wdd_FailMsg("DirectDraw","Could not attach z buffer.",ddrval);
            wdd_Log("d3d init: could not attach zbuf\n");
            wdd_Kill3D(wdd);
            return(FALSE);
        };
    };

    /*** Erzeuge 3D-Device ***/
    ddrval = lpD3D2->lpVtbl->CreateDevice(lpD3D2,&(wdd_Data.Driver.DevGuid),
                                          wdd->lpDDSBack, &(wdd->d3d->lpD3DDevice2));
    if (ddrval != DD_OK) {
        wdd_FailMsg("windd.class","IDirect3D2::CreateDevice() failed.",ddrval);
        return(FALSE);
    };
    ddrval = wdd->d3d->lpD3DDevice2->lpVtbl->QueryInterface(wdd->d3d->lpD3DDevice2,
                       &IID_IDirect3DDevice,&(wdd->d3d->lpD3DDevice));
    if (ddrval != DD_OK) {
        wdd_FailMsg("windd.class","IDirect3DDevice2::QueryInterface(IID_IDirect3DDevice) failed",ddrval);
        return(FALSE);
    };    
    
    // *** OBSOLETE *** OBSOLETE *** OBSOLETE ***    
    //ddrval = wdd->lpDDSBack->lpVtbl->QueryInterface(wdd->lpDDSBack,
    //              &(wdd_Data.Driver.DevGuid), &(wdd->d3d->lpD3DDevice));
    //if (ddrval != DD_OK) {
    //    wdd_FailMsg("DirectDraw","Couldn't create 3d device.",ddrval);
    //    wdd_Kill3D(wdd);
    //    wdd_Log("d3d init: could not create d3d device\n");
    //    return(FALSE);
    //};

    /*** enumeriere Textur-Formate ***/
    wdd->d3d->NumTxtFormats = 0;
    ddrval = wdd->d3d->lpD3DDevice2->lpVtbl->EnumTextureFormats(
                wdd->d3d->lpD3DDevice2,
                wdd_EnumTextureFormatsCallback,
                (LPVOID)wdd);
    if (ddrval != DD_OK) {
        wdd_FailMsg("DirectDraw","EnumTextureFormats failed.",ddrval);
        wdd_Log("d3d init: EnumTextureFormats() failed\n");
        wdd_Kill3D(wdd);
        return(FALSE);
    };

    /*** SurfaceDesc der Primary Surface holen ***/
    memset(&ddsd,0,sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddrval = wdd->lpDDSPrim->lpVtbl->GetSurfaceDesc(wdd->lpDDSPrim,&ddsd);
    if (ddrval != DD_OK) {
        wdd_FailMsg("DirectDraw","GetSurfaceDesc(Primary)",ddrval);
        wdd_Log("d3d init: GetSurfaceDesc() for primary failed\n");
        wdd_Kill3D(wdd);
        return(FALSE);
    };

    /*** suche geeignetes Textur-Format ***/
    wdd->d3d->ActTxtFormat  = -1;
    txt_8bpp_fit  = -1;
    txt_16bpp_fit = -1;
    txt_32bpp_fit = -1;
    for (i=0; i<wdd->d3d->NumTxtFormats; i++) {
        struct wdd_TxtFormat *t = &(wdd->d3d->TxtFormats[i]);
        struct wdd_3DDriver *d = &(wdd_Data.Driver);
        unsigned long txt_r_mask = t->ddsd.ddpfPixelFormat.dwRBitMask;
        unsigned long txt_g_mask = t->ddsd.ddpfPixelFormat.dwGBitMask;
        unsigned long txt_b_mask = t->ddsd.ddpfPixelFormat.dwBBitMask;
        unsigned long txt_a_mask = t->ddsd.ddpfPixelFormat.dwRGBAlphaBitMask;

        if ((d->CanDoAlpha) && (d->CanDoAdditiveBlend) &&
            (d->CanDoColorKeyTxt) && (t->IndexBPP == 8) && (txt_8bpp_fit == -1))
        {
            /*** der Alphablending-Idealfall ***/
            txt_8bpp_fit = i;
        } else if ((d->CanDoStipple) && (d->CanDoColorKeyTxt) &&
                   (t->IndexBPP == 8) && (txt_8bpp_fit == -1))
        {
            /*** der Stipple-Idealfall ***/
            txt_8bpp_fit = i;
        } else if ((t->RedBPP   == 4) &&
                   (t->GreenBPP == 4) &&
                   (t->BlueBPP  == 4) &&
                   (t->AlphaBPP == 4))
        {
            /*** an 16 Bit nur 4444 ***/
            txt_16bpp_fit = i;
        } else if ((t->RedBPP   == 8) &&
                   (t->GreenBPP == 8) &&
                   (t->BlueBPP  == 8) &&
                   (t->AlphaBPP == 8))
        {
            /*** an 32 Bit nur 8888 ***/
            txt_32bpp_fit = i;
        };
    };
    if (txt_8bpp_fit != -1) {
        wdd->d3d->ActTxtFormat = txt_8bpp_fit;
        wdd_Log("d3d init: use 8bpp clut texture format\n");
    } else if (txt_16bpp_fit != -1) {
        wdd->d3d->ActTxtFormat = txt_16bpp_fit;
        wdd_Log("d3d init: use 4444 texture format\n");
    } else if (txt_32bpp_fit != -1) {
        wdd->d3d->ActTxtFormat = txt_32bpp_fit;
        wdd_Log("d3d init: use 8888 texture format\n");
    } else {
        wdd_FailMsg("Direct3D","No suitable texture format.",DD_OK);
        wdd_Log("d3d init: no suitable texture format found\n");
        wdd_Kill3D(wdd);
        return(FALSE);
    };

    /*** Viewport erzeugen ***/
    ddrval = lpD3D2->lpVtbl->CreateViewport(lpD3D2,&(wdd->d3d->lpD3DViewport),NULL);
    if (ddrval != D3D_OK) {
        wdd_FailMsg("windd.class","IDirect3D::CreateViewport()",ddrval);
        wdd_Kill3D(wdd);
        return(FALSE);
    };
    /*** Viewport an D3D-Device attachen ***/
    ddrval = wdd->d3d->lpD3DDevice2->lpVtbl->AddViewport(
             wdd->d3d->lpD3DDevice2,wdd->d3d->lpD3DViewport);
    if (ddrval != D3D_OK) {
        wdd_FailMsg("windd.class","IDirect3DDevice::AddViewport()",ddrval);
        wdd_Kill3D(wdd);
        return(FALSE);
    };
    /*** Viewport initialisieren ***/
    memset(&vport,0,sizeof(D3DVIEWPORT));
    vport.dwSize = sizeof(D3DVIEWPORT);
    vport.dwX = 0;
    vport.dwY = 0;
    vport.dwWidth  = w;
    vport.dwHeight = h;
    vport.dvScaleX = vport.dwWidth / (float)2.0;
    vport.dvScaleY = vport.dwHeight / (float)2.0;
    vport.dvMaxX = (float)D3DDivide(D3DVAL(vport.dwWidth),
                                    D3DVAL(2*vport.dvScaleX));
    vport.dvMaxY = (float)D3DDivide(D3DVAL(vport.dwHeight),
                                    D3DVAL(2*vport.dvScaleY));
    ddrval = wdd->d3d->lpD3DViewport->lpVtbl->SetViewport(
             wdd->d3d->lpD3DViewport,&vport);
    if (ddrval != D3D_OK) {
        wdd_FailMsg("windd.class","IDirect3DViewport::SetViewport()",ddrval);
        wdd_Kill3D(wdd);
        return(FALSE);
    };
    ddrval = wdd->d3d->lpD3DDevice2->lpVtbl->SetCurrentViewport(
             wdd->d3d->lpD3DDevice2, wdd->d3d->lpD3DViewport);
    if (ddrval != D3D_OK) {
        wdd_FailMsg("windd.class","ID3DDevice2::SetCurrentViewport() failed",ddrval);
        wdd_Kill3D(wdd);
        return(FALSE);
    };    

    /*** Material für Background (schwarz) und an Viewport hängen ***/
    ddrval = lpD3D2->lpVtbl->CreateMaterial(lpD3D2,&(wdd->d3d->lpBackMat),NULL);
    if (ddrval != D3D_OK) {
        wdd_FailMsg("Direct3D","IDirect3D::CreateMaterial(Background)",ddrval);
        wdd_Kill3D(wdd);
        return(FALSE);
    };
    memset(&bmat,0,sizeof(D3DMATERIAL));
    bmat.dwSize    = sizeof(D3DMATERIAL);
    bmat.diffuse.r = (D3DVALUE) 0.0;
    bmat.diffuse.g = (D3DVALUE) 0.0;
    bmat.diffuse.b = (D3DVALUE) 0.0;
    bmat.diffuse.a = (D3DVALUE) 0.0;
    bmat.dwRampSize = 1;
    wdd->d3d->lpBackMat->lpVtbl->SetMaterial(wdd->d3d->lpBackMat,&bmat);
    wdd->d3d->lpBackMat->lpVtbl->GetHandle(wdd->d3d->lpBackMat,wdd->d3d->lpD3DDevice2,&hbmat);
    wdd->d3d->lpD3DViewport->lpVtbl->SetBackground(wdd->d3d->lpD3DViewport,hbmat);

    /*** erzeuge Execute Buffer ***/
    exbuf_size = 65536;
    if ((wdd_Data.Driver.Desc.dwMaxBufferSize > 0) &&
        (wdd_Data.Driver.Desc.dwMaxBufferSize<exbuf_size))
    {
        exbuf_size = wdd_Data.Driver.Desc.dwMaxBufferSize;
    };
    memset(&exbuf_desc,0,sizeof(D3DEXECUTEBUFFERDESC));
    exbuf_desc.dwSize = sizeof(D3DEXECUTEBUFFERDESC);
    exbuf_desc.dwFlags = D3DDEB_BUFSIZE;
    exbuf_desc.dwBufferSize = exbuf_size;
    wdd_Log("d3d init: create execbuf with size %d\n",exbuf_size);
    ddrval = wdd->d3d->lpD3DDevice->lpVtbl->CreateExecuteBuffer(
             wdd->d3d->lpD3DDevice,
             &exbuf_desc,
             &(wdd->d3d->lpD3DExecBuffer),
             NULL);
    if (ddrval != D3D_OK) {
        wdd_FailMsg("wdd_Create3D","CreateExecuteBuffer() failed.",ddrval);
        wdd_Log("d3d init: CreateExecuteBuffer() failed.\n");
        wdd_Kill3D(wdd);
        return(FALSE);
    };
    
    if (wdd->usedrawprimitive) wdd_Log("***> using DrawPrimitive <***\n");
    else                       wdd_Log("***> using ExecuteBuffer <***\n");        

    /*** alles ok... ***/
    wdd_Log("-> Leaving wdd_Create3D()\n");
    return(TRUE);
}

/*-----------------------------------------------------------------*/
unsigned long wdd_InitDDrawStuff(struct windd_data *wdd,
                                 char *cm,
                                 unsigned long w,
                                 unsigned long h,
                                 unsigned long bpp)
/*
**  FUNCTION
**      Initialisiert komplett das Display-Environment für
**      2D- und 3D-Modus.
**
**  INPUTS
**      wdd         - Pointer auf LID des windd.class Objekt
**      cm          - UBYTE * auf 256 RGB-Entries,
**                    oder NULL
**      w,h,bpp     - gewollte Höhe, Breite und Bittiefe
**                    der Surface in Pixel
**
**  RESULTS
**      TRUE  - alles OK
**      FALSE - ein Fehler (MessageBox() wurde bereits invoked)
**
**      Im Fall eines Fehlers muß wdd_KillDDrawStuff()
**      aufgerufen werden.
**
**  CHANGED
**      11-Nov-96   floh    created
**      22-Nov-96   floh    falls das globale MainWindow bereits
**                          einmal geöffnet wurde, wird es nicht
**                          neu erzeugt, sondern nur resized!
**      26-Jan-97   floh    + wertet jetzt das WINDDF_IsWindowed-Flag
**                            aus
**                          + schreibt Größe des Backbuffers nach
**                            wdd->back_h, wdd->back_h
**      28-Jan-97   floh    + korrektes Handling für Fullscreen-Modes
**      03-Feb-97   floh    + veraendertes Fullscreen Handling
**      14-Feb-97   floh    + Backsurface generell im Sysmem
**                          + Window erzeugen + modifizieren ausgelagert
**                          + Windowed-Modus extrem downgraded (nur noch
**                            für Debugging interessant)
**      05-Mar-97   floh    + initialisiert jetzt bei Bedarf Direct3D-
**                            Umgebung
*/
{
    HINSTANCE inst;
    int cmd_show;
    PALETTEENTRY  pal[256];
    HRESULT       ddrval;
    
    wdd_Log("-> Entering wdd_InitDDrawStuff()\n");    

    /*** globale Variablen aus win_main.c ***/
    inst = win_Instance;
    if (NULL == inst) return(FALSE);
    cmd_show = win_CmdShow;
    if (NULL == cmd_show) return(FALSE);

    /*** Colormap validieren ***/
    if (cm) {
        /*** konvertiere UBYTE[3] Array in Win-Format ***/
        int i;
        for (i=0; i<256; i++) {
            pal[i].peRed   = *cm++;
            pal[i].peGreen = *cm++;
            pal[i].peBlue  = *cm++;
            pal[i].peFlags = 0;
        };
    } else {
        /*** eine Default-332-Palette initialisieren ***/
        int i;
        for (i=0; i<256; i++) {
            pal[i].peRed   = (BYTE)(((i>>5) & 0x7) * 255 / 7);
            pal[i].peGreen = (BYTE)(((i>>2) & 0x7) * 255 / 7);
            pal[i].peBlue  = (BYTE)(((i>>0) & 0x3) * 255 / 7);
            pal[i].peFlags = 0;
        };
    };

    /*** und los... ***/
    wdd->hInstance    = inst;
    wdd->back_w       = w;
    wdd->back_h       = h;
    wdd->cur_ptr_type = -1L; // dont ask...

    /*** Window validieren ***/
    if (!wdd_ValidateWindow(wdd,inst,cmd_show,w,h)) {
        wdd_Log("wdd_ValidateWindow() failed.\n");
        return(FALSE);
    };
    wdd_Log("->     after wdd_ValidateWindow()\n");

    /*** Direct3D bzw DirectDraw initialisieren ***/
    if (wdd_DoDirect3D) {

        if (wdd->flags & WINDDF_IsWindowed) {
            /*** 3D: Windowed Modus initialisieren ***/
            if (!wdd_Create3DWinEnv(wdd,w,h,bpp)) return(FALSE);
        } else {
            /*** 3D: Fullscreen Modus initialisieren ***/
            if (!wdd_Create3DFullEnv(wdd,w,h,bpp)) return(FALSE);
        };
        wdd_Log("->     after wdd_Create3DFull/WinEnv()\n");

        /*** gemeinsame (Full/Win) Direct3D-Initialisierung ***/
        if (!wdd_Create3D(wdd,w,h,bpp)) return(FALSE);
        wdd_Log("->     after wdd_Create3D()\n");

    } else {
        /*** DDraw-Display initialisieren ***/
        if (wdd->flags & WINDDF_IsWindowed) {
            /*** WINDOWED MODE (DEBUGGING ONLY) ***/
            if (!wdd_Create2DWinEnv(wdd,pal,w,h)) return(FALSE);
        } else {
            /*** FULLSCREEN MODE ***/
            if (!wdd_Create2DFullEnv(wdd,pal,w,h)) return(FALSE);
        };
        wdd_Log("->     after wdd_Create2DFull/WinEnv()\n");    
    };

        /*** falls DBCS Version, das DBCS Modul initialisieren ***/
        #ifdef __DBCS__
                wdd_Data.DBCSHandle = dbcs_Init(lpDD,wdd->lpDDSBack,"MS Sans Serif,12,400,0");
                wdd_Log("->     after dbcs_Init()\n");
        #endif

    /*** SurfaceDesc der Primary Surface holen ***/
    memset(&(wdd_Data.Primary),0,sizeof(wdd_Data.Primary));
    wdd_Data.Primary.dwSize = sizeof(wdd_Data.Primary);
    ddrval = wdd->lpDDSPrim->lpVtbl->GetSurfaceDesc(wdd->lpDDSPrim,&(wdd_Data.Primary));
    if (ddrval != DD_OK) {
       wdd_FailMsg("windd.class/wdd_winbox.c","IDirectDrawSurface::GetSurfaceDesc(Primary)",ddrval);
       return(FALSE);
    };
    wdd_Log("->     after GetSurfaceDesc()\n");

    /*** System-Cursor initial verstecken, falls Software-Rendering ***/
    if (wdd_DoSoftCursor(wdd)) ShowCursor(FALSE);
    
    wdd_Log("-> Leaving wdd_InitDrawStuff()\n");    

    /*** alles OK ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void wdd_KillDDrawStuff(struct windd_data *wdd)
/*
**  FUNCTION
**      Gegenstück zu wdd_InitDDrawStuff()
**
**  CHANGED
**      11-Nov-96   floh    created
**      22-Nov-96   floh    DestroyWindow() wird NICHT mehr
**                          aufgerufen! Damit bleibt das einmalige
**                          globale Window bestehen, auch wenn
**                          das windd.class Objekt gekillt wird!
**      19-Nov-97   floh    DBCS Deinitialisierung
**      06-Apr-98   floh    + löscht Primary- und Backsurface, um
**                            Display-Artefakte beim Aufräumen zu
**                            verhindern.
*/
{
    wdd_Log("-> Entering wdd_KillDDrawStuff()\n");

    #ifdef __DBCS__
        dbcs_Kill();
        wdd_Data.DBCSHandle = NULL;
    #endif

    if (!(wdd->flags & WINDDF_IsWindowed)) {
        lpDD->lpVtbl->RestoreDisplayMode(lpDD);
    };

    /*** das Window bleibt zwar am Leben, alles andere geht ***/
    /*** aber drauf! ***/
    if (wdd->hWnd) {

        DDBLTFX ddbltfx;
        HRESULT ddrval;

        /*** lösche Primary und Backsurface ***/
        ddbltfx.dwSize      = sizeof(ddbltfx);
        ddbltfx.dwFillColor = 0;
		if (wdd->lpDDSPrim)
		{
			ddrval = wdd->lpDDSPrim->lpVtbl->Blt(wdd->lpDDSPrim,
						NULL, NULL, NULL, DDBLT_COLORFILL|DDBLT_WAIT,
						&ddbltfx);
		}
		if (wdd->lpDDSBack)
		{
			ddrval = wdd->lpDDSBack->lpVtbl->Blt(wdd->lpDDSBack,
						NULL, NULL, NULL, DDBLT_COLORFILL|DDBLT_WAIT,
						&ddbltfx);
		}

        /*** damit die WinProc nicht Amok läuft... ***/
        SetClassLong(wdd->hWnd,0,NULL);

        /*** alles killen! ***/
        wdd_KillDDrawObjects(wdd);
        wdd->hWnd = NULL;
    };
    wdd->hInstance = NULL;
    wdd_Log("-> Leaving wdd_KillDDrawStuff()\n");
}

/*-----------------------------------------------------------------*/
void wdd_Begin(struct windd_data *wdd)
/*
**  FUNCTION
**      Wird innerhalb DISPM_Begin ausgeführt, stellt
**      sicher, das die Backsurface existiert und
**      lockt diese und returniert einen Pointer
**      auf den Rendering-Bereich der Surface.
**
**  CHANGED
**      11-Nov-96   floh    created
**      14-Nov-96   floh    umgeschrieben für Fullscreen
**      20-Nov-96   floh    jetzt sowohl für Fullscreen-
**                          als auch für Windowed-Modes
**                          (hmm, waren gar keine Änderungen
**                          notwendig... oder?)
**      03-Feb-97   floh    Pitch jetzt korrekt
**      15-Feb-97   floh    Es kann vorkommen, daß die Routine
**                          aufgerufen wird, wenn das Fenster
**                          gar nicht mehr existiert. Dieser
**                          Fall wird jetzt beachtet.
**      19-Nov-97   floh    + benutzt jetzt testweise das DDLOCK_NOSYSLOCK
**                          Flag beim Locken der Backsurface
*/
{
    HRESULT ddrval;
    POINT p;

    /*** Mouse-Ruckel-Hack ***/
    GetCursorPos(&p);
    SetCursorPos(p.x,p.y);

    /*** Window-Handle noch gültig? ***/
    if (wdd->hWnd) {

        /*** Lost->Restore ***/
        wdd_CheckLostSurfaces(wdd);

        /*** Backsurface löschen ***/
        wdd_Clear(wdd);

        if (!wdd_DoDirect3D) {
            /*** DirectDraw: Lock auf Back-Surface (etwas Brute Force...) ***/
            DDSURFACEDESC ddsd;

            memset(&ddsd,0,sizeof(ddsd));
            ddsd.dwSize = sizeof(ddsd);
            ddrval = wdd->lpDDSBack->lpVtbl->Lock(wdd->lpDDSBack,NULL,&ddsd,
                                                  DDLOCK_NOSYSLOCK|DDLOCK_WAIT,NULL);
            if (ddrval == DD_OK) {
                wdd->back_ptr   = ddsd.lpSurface;
                wdd->back_pitch = ddsd.lPitch;
            } else {
                wdd->back_ptr   = NULL;
                wdd->back_pitch = NULL;
                wdd_Log("-> wdd_Begin(): Lock on primary surface failed.\n");
            };
        } else {
            wdd->back_ptr   = NULL;
            wdd->back_pitch = NULL;
        };
    } else {
        /*** kein Window-Handle mehr ***/
        wdd->back_ptr   = NULL;
        wdd->back_pitch = NULL;
    };
}

/*-----------------------------------------------------------------*/
void wdd_End(struct windd_data *wdd)
/*
**  FUNCTION
**      Unlockt Back-Surface und flippt Buffers.
**
**  CHANGED
**      11-Nov-96   floh    created
**      14-Nov-96   floh    umgeschrieben für Fullscreen
**      20-Nov-96   floh    arbeitet jetzt sowohl Fullscreen,
**                          als auch Windowed
**      14-Feb-97   floh    Windowed-Modus nur noch 8 Bit
**      15-Feb-97   floh    ungültiges Window-Handle wird
**                          abgefangen
*/
{
    /*** Window noch gültig? ***/
    if (wdd->hWnd) {

        HRESULT ddrval;
        POINT p;

        /*** Backsurface unlocken ***/
        if (!wdd_DoDirect3D) {
            ddrval = wdd->lpDDSBack->lpVtbl->Unlock(wdd->lpDDSBack,NULL);
        };
        if (wdd->flags & WINDDF_IsWindowed) {

            /*** Window-Mode: etwas mehr Arbeit ***/
            RECT dest_r;
            POINT pt;

            GetClientRect(wdd->hWnd,&dest_r);
            pt.x = 0;
            pt.y = 0;
            ClientToScreen(wdd->hWnd,&pt);
            OffsetRect(&dest_r,pt.x,pt.y);
            ddrval = wdd->lpDDSPrim->lpVtbl->Blt(wdd->lpDDSPrim,
                     &dest_r, wdd->lpDDSBack, NULL, DDBLT_WAIT, NULL);

        } else {
            /*** Fullscreen: SysMemBackBuf -> Primary ***/
            if (wdd_DoDirect3D) {
                /*** 3D-Modus: eine Complex-Flipping-Surface ***/
                ddrval = wdd->lpDDSPrim->lpVtbl->Flip(
                         wdd->lpDDSPrim,wdd->lpDDSBack,DDFLIP_WAIT);
            } else {
                /*** sonst Blit, weil in SysMem ***/
                ddrval = wdd->lpDDSPrim->lpVtbl->Blt(wdd->lpDDSPrim,NULL,
                         wdd->lpDDSBack,NULL,DDBLT_WAIT,NULL);
            };
        };
    };
}

/*-----------------------------------------------------------------*/
void wdd_SetPalette(struct windd_data *wdd, char *cm)
/*
**  CHANGED
**      22-Nov-96   floh    created
*/
{
    if (wdd->lpDDPal) {

        PALETTEENTRY  pal[256];
        int i;

        /*** konvertiere UBYTE[3] Array in Win-Format ***/
        for (i=0; i<256; i++) {
            pal[i].peRed   = *cm++;
            pal[i].peGreen = *cm++;
            pal[i].peBlue  = *cm++;
            pal[i].peFlags = 0;
        };
        wdd->lpDDPal->lpVtbl->SetEntries(wdd->lpDDPal,0,0,256,pal);
        wdd->lpDDSPrim->lpVtbl->SetPalette(wdd->lpDDSPrim,wdd->lpDDPal);
    };
}

/*-----------------------------------------------------------------*/
void wdd_SetMouseImage(struct windd_data *wdd,
                       unsigned long type,
                       unsigned long flush)
/*
**  FUNCTION
**      Setzt Mauszeiger, folgende Resourcen müssen
**      vorhanden sein:
**
**          Attack.cur
**          Cancel.cur
**          Disk.cur
**          Pointer.cur
**          Select.cur
**
**  INPUTS
**      wdd
**      type    -   0: Mauspointer löschen
**                  1: normaler Pointer
**                  2: Attack
**                  3: Cancel
**                  4: Disk Access
**                  5: Select
**                  6: Action
**      flush   -   TRUE  -> Cursor auf alle Fälle refreshen, dabei
**                           den Typ ignorieren (selben Typ wie vorher
**                           nehmen)
**                  FALSE -> Cursor nur ändern, wenn vorher ein
**                           anderer Cursor eingestellt war.
**
**  
**  CHANGED
**      22-Nov-96   floh    created
**      15-Feb-97   floh    beachtet ungültiges Window-Handle
**      13-Mar-98   floh    + checkt jetzt selbst auf DoSoftCursor
**      31-May-98   floh    + oops, beim Flush-Modus wurde nicht der
**                            type aus cur_ptr_type genommen...
*/
{
    if (wdd->hWnd && (!wdd_DoSoftCursor(wdd))) {

        unsigned long reload = FALSE;

        if (flush) {
            /*** Flush: aktuellen Cursor sichtbar machen ***/
            wdd->cur_ptr_type = type;
            while (ShowCursor(TRUE)<0);
            reload = TRUE;
        } else if (type != wdd->cur_ptr_type) {
            /*** Cursor verstecken? ***/
            if (wdd->cur_ptr_type && (!type)) while (ShowCursor(FALSE)>=0);
            /*** Cursor wiederherstellen? ***/
            if ((!wdd->cur_ptr_type) && type) while (ShowCursor(TRUE)<0);
            wdd->cur_ptr_type = type;
            reload = TRUE;
        };

        /*** Cursorimage ändern? ***/
        if (reload) {
            HCURSOR hCursor = NULL;
            switch (type) {
                case 1:  hCursor=LoadCursor(wdd->hInstance,"Pointer"); break;
                case 2:  hCursor=LoadCursor(wdd->hInstance,"Cancel"); break;
                case 3:  hCursor=LoadCursor(wdd->hInstance,"Select"); break;
                case 4:  hCursor=LoadCursor(wdd->hInstance,"Attack"); break;
                case 5:  hCursor=LoadCursor(wdd->hInstance,"Goto"); break;
                case 6:  hCursor=LoadCursor(wdd->hInstance,"Disk"); break;
                case 7:  hCursor=LoadCursor(wdd->hInstance,"New"); break;
                case 8:  hCursor=LoadCursor(wdd->hInstance,"Add"); break;
                case 9:  hCursor=LoadCursor(wdd->hInstance,"Control"); break;
                case 10: hCursor=LoadCursor(wdd->hInstance,"Beam"); break;
                case 11: hCursor=LoadCursor(wdd->hInstance,"Build"); break;
            };
            if (hCursor) SetCursor(hCursor);
        };
    };
}           

/*-----------------------------------------------------------------*/
void wdd_EnableGDI(struct windd_data *wdd, unsigned long mode)
/*
**  FUNCTION
**      Unternimmt alle notwendigen Aktionen, damit
**      das GDI in die Primary Surface rendern kann.
**
**  INPUT
**      mode    -   WINDD_GDIMODE_WINDOW
**                  WINDD_GDIMODE_MOVIE
**
**  CHANGED
**      08-Apr-97   floh    created
**      30-Sep-97   floh    + wenn SoftCursor Modus, wird der
**                            Systemcursor eingeblendet.
**      07-Jan-98   floh    + erzeugt jetzt eine Backup-Surface mit
**                            dem aktuellen Inhalt des Backbuffers
**      03-Feb-98   floh    + Auflösungs-Hochschalten temporär
**                            rausgenommen, macht neuerdings
**                            Probleme.
**      04-Feb-98   floh    + More Hacks für Movieplayer
**      26-Feb-98   floh    + für Movieplayer wird generell auf
**                            640x480x16 geschaltet
**      25-May-98   floh    + im Movieplayer-Modus wird Bildschirm
**                            vorher geloescht.
**      03-Jun-98   floh    + oops, im Software-Mouse-Modus wurde die
**                            Mouse weggeloescht!
*/
{
    if (wdd->hWnd) {

        HRESULT ddrval;
        DDBLTFX ddbltfx;
        HCURSOR hCursor = NULL;

        wdd_DuringEnableGDI = TRUE;
        
        /*** im DirectDraw Modus zuerst Backsurface unlocken ***/
        if (!wdd_DoDirect3D) {
            ddrval = wdd->lpDDSBack->lpVtbl->Unlock(wdd->lpDDSBack,NULL);
            wdd->back_ptr   = NULL;
            wdd->back_pitch = 0;
        };

        /*** erzeuge Kopie des Backbuffers ***/
        ddrval = wdd_CreateBackupSurface(wdd);

        if ((wdd->flags & WINDDF_IsWindowed) == 0) {

            /*** ein Fullscreen Mode ***/
            if (wdd_DoDirect3D && (!(wdd_Data.Flags & WINDDF_CanDoWindowed))) {
                /*** eine Fullscreen-Karte ***/
                ddrval = lpDD->lpVtbl->FlipToGDISurface(lpDD);
                delay(100);
            };

            /*** Lowres? Dann lieber etwas hoch schalten ***/
            if (mode == WINDD_GDIMODE_WINDOW) {
                /*** im GDI-Window-Modus, Auflösung evtl. hochschalten ***/
                if ((wdd_Data.Primary.dwWidth <= 400) ||
                    (wdd_Data.Primary.dwHeight <= 300))
                {
                    ddrval = lpDD->lpVtbl->SetDisplayMode(lpDD,
                                wdd_Data.Desktop.dwWidth,
                                wdd_Data.Desktop.dwHeight,
                                wdd_Data.Desktop.ddpfPixelFormat.dwRGBBitCount);
                };
                ddrval = lpDD->lpVtbl->SetCooperativeLevel(lpDD,wdd->hWnd,DDSCL_NORMAL);
                SetWindowLong(wdd->hWnd, GWL_STYLE, WS_POPUP|WS_SYSMENU);
                SetWindowLong(wdd->hWnd, GWL_EXSTYLE, WS_EX_TOPMOST);
                SetWindowPos(wdd->hWnd, 0, 0, 0,
                             GetSystemMetrics(SM_CXSCREEN),
                             GetSystemMetrics(SM_CYSCREEN),
                             SWP_SHOWWINDOW);
                wdd_BltBgDC(wdd);
            } else {
                
                DDBLTFX ddbltfx;                
                
                /*** Movieplayer -> Surfaces loeschen ***/
                ddbltfx.dwSize      = sizeof(ddbltfx);
                ddbltfx.dwFillColor = 0;
                ddrval = wdd->lpDDSPrim->lpVtbl->Blt(wdd->lpDDSPrim,
                            NULL, NULL, NULL, DDBLT_COLORFILL|DDBLT_WAIT,
                            &ddbltfx);
                ddrval = wdd->lpDDSBack->lpVtbl->Blt(wdd->lpDDSBack,
                            NULL, NULL, NULL, DDBLT_COLORFILL|DDBLT_WAIT,
                            &ddbltfx);
                                
                /*** Movie-Player: Auflösung auf 640x480 ***/
                ddrval = lpDD->lpVtbl->SetDisplayMode(lpDD, 640, 480, 16);
                ddrval = lpDD->lpVtbl->SetCooperativeLevel(lpDD,wdd->hWnd,DDSCL_NORMAL);
                SetWindowLong(wdd->hWnd, GWL_STYLE, WS_POPUP|WS_SYSMENU);
                SetWindowLong(wdd->hWnd, GWL_EXSTYLE, WS_EX_TOPMOST);
                SetWindowPos(wdd->hWnd, 0, 0, 0,
                             GetSystemMetrics(SM_CXSCREEN),
                             GetSystemMetrics(SM_CYSCREEN),
                             SWP_SHOWWINDOW);
            };

        };
        /*** Systemcursor laden ***/
        while (ShowCursor(TRUE)<0);
        hCursor=LoadCursor(wdd->hInstance,"Pointer");
        if (hCursor) SetCursor(hCursor);
    };
}

/*-----------------------------------------------------------------*/
void wdd_DisableGDI(struct windd_data *wdd, unsigned long mode)
/*
**  FUNCTION
**      Macht wdd_EnableGDI wieder rückgängig.
**
**  INPUT
**      mode    -   WINDD_GDIMODE_WINDOW
**                  WINDD_GDIMODE_MOVIE
**
**  CHANGED
**      08-Apr-97   floh    created
**      16-Aug-97   floh    + Palette wird frisch gesetzt
**      30-Sep-97   floh    + wenn SoftCursorModus, wird Cursor
**                            wieder ausgeblendet.
**      07-Jan-98   floh    + killt Backup-Puffer wieder
**      30-Jan-98   floh    + wdd->cur_ptr_type wird auf NULL gesetzt,
**                            damit nach dem DisableGDI wieder der
**                            Standard-Cursor gesetzt wird
**      03-Feb-98   floh    + Auflösungs-Umschalten temporär rausgenommen.
**      04-Feb-98   floh    + more hacks für Movieplayer
**      26-Feb-98   floh    + im Movie-Mode Auflösung zurückschalten
**      13-Mar-98   floh    + Cursor wird jetzt sicherer restauriert
*/
{
    if (wdd->hWnd) {

        HRESULT ddrval;
        DDBLTFX ddbltfx;

        wdd_DuringEnableGDI = FALSE;
        wdd_DestroyBackupSurface(wdd);
        if (wdd_DoSoftCursor(wdd)) {
            /*** Software-Cursor: System-Pointer ausblenden ***/
            ShowCursor(FALSE);
        };

        /*** Auflösung runterschalten ? ***/
        if ((wdd->flags & WINDDF_IsWindowed) == 0) {

            unsigned long switch_display = FALSE;
            ddrval = lpDD->lpVtbl->SetCooperativeLevel(lpDD,wdd->hWnd,DDSCL_EXCLUSIVE|DDSCL_FULLSCREEN);

            if (mode == WINDD_GDIMODE_WINDOW){
                if ((wdd_Data.Primary.dwWidth <= 400) ||
                    (wdd_Data.Primary.dwHeight <= 300))
                {
                    switch_display = TRUE;
                };
            } else {
                /*** im Movie-Mode, Auflösung generell zurückschalten und Buffer loeschen ***/
                switch_display = TRUE;
            };

            /*** Auflösung wieder umschalten? ***/
            if (switch_display) {

                /*** Displaymode umschalten ***/
                ddrval = lpDD->lpVtbl->SetDisplayMode(lpDD,
                            wdd_Data.Primary.dwWidth,
                            wdd_Data.Primary.dwHeight,
                            wdd_Data.Primary.ddpfPixelFormat.dwRGBBitCount);
                SetWindowLong(wdd->hWnd, GWL_STYLE, WS_POPUP|WS_SYSMENU);
                SetWindowLong(wdd->hWnd, GWL_EXSTYLE, WS_EX_TOPMOST);
                SetWindowPos(wdd->hWnd, 0, 0, 0,
                             GetSystemMetrics(SM_CXSCREEN),
                             GetSystemMetrics(SM_CYSCREEN),
                             SWP_SHOWWINDOW);
            };
        };
        /*** Mousepointer mit Flush neu setzen ***/
        wdd_SetMouseImage(wdd,wdd->cur_ptr_type,TRUE);
    };
}

/*-----------------------------------------------------------------*/
void wdd_WrongDisplayDepth(struct windd_data *wdd)
/*
**  FUNCTION
**      Zeigt eine MessageBox() an, daß windowed Mode
**      in dieser Bittiefe nicht supportet ist.
**
**  CHANGED
**      08-Apr-97   floh    created
*/
{
    MessageBox(NULL,"Windowed mode not supported\nin current desktop bit depth.",
               "YPA Message",0);
}

/*-----------------------------------------------------------------*/
void wdd_GetMousePos(struct windd_data *wdd, long *x, long *y)
/*
**  CHANGED
**      14-Apr-97   floh    created
*/
{
    POINT p;
    GetCursorPos(&p);
    *x = (p.x * wdd->back_w) / GetSystemMetrics(SM_CXSCREEN);
    *y = (p.y * wdd->back_h) / GetSystemMetrics(SM_CYSCREEN);
}

/*-----------------------------------------------------------------*/
void wdd_PlayMovie(struct windd_data *wdd, char *fname)
/*
**  CHANGED
**      22-Jan-98   floh    created
*/
{
    if (wdd->hWnd) {
        wdd_MoviePlaying = TRUE;
        wdd_EnableGDI(wdd,WINDD_GDIMODE_MOVIE);
        if (wdd->movieplayer) dshow_PlayMovie(fname,wdd->hWnd);
        wdd_DisableGDI(wdd,WINDD_GDIMODE_MOVIE);
        wdd_MoviePlaying = FALSE;
    };
}

/*-----------------------------------------------------------------*/
void wdd_SetCursorMode(struct windd_data *wdd, unsigned long mode)
/*
**  CHANGED
**      23-Feb-98   floh    created
*/
{
    if ((WINDD_CURSORMODE_HW==mode) && wdd->forcesoftcursor) {
        /*** nach System-Pointer umschalten ***/
        wdd->forcesoftcursor = FALSE;
        while (ShowCursor(TRUE)<0);
        wdd_SetMouseImage(wdd,1,TRUE);
    } else if ((WINDD_CURSORMODE_SOFT==mode) && (!wdd->forcesoftcursor)) {
        /*** nach Soft-Pointer umschalten ***/
        wdd->forcesoftcursor = TRUE;
        while (ShowCursor(FALSE)>=0);
    };
}

