#ifndef BITMAP_DISPLAYCLASS_H
#define BITMAP_DISPLAYCLASS_H
/*
**  $Source: PRG:VFM/Include/bitmap/displayclass.h,v $
**  $Revision: 38.8 $
**  $Date: 1998/01/06 12:54:12 $
**  $Locker:  $
**  $Author: floh $
**
**  display.class
**
**  (C) Copyright 1996 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif

#ifndef BITMAP_RASTERCLASS_H
#include "bitmap/rasterclass.h"
#endif

/*-------------------------------------------------------------------
**  NAME
**      display.class -- Stammklasse für Display-Treiber-Klassen
**
**  FUNCTION
**      Die display.class ergänzt die raster.class um Funktionen
**      zur Ansteuerung eines "Display-Devices". Man erzeugt
**      im Grunde eine "anzeigbare VFMBitmap, in die man
**      malen kann".
**
**  METHODS
**      DISPM_Query
**          Msg:    struct disp_query_msg
**          Ret:    ULONG
**
**          Gibt Informationen über die in der Message
**          definierte DisplayID zurück. Die Informationen
**          werden in die <disp_query_msg> geschrieben.
**
**          Der Return-Value entspricht der nächsten
**          verfügbaren Display-ID, oder 0L, wenn
**          keine weiteren Display-IDs angeboten werden.
**
**          Falls das DisplayID-Feld in der Message
**          auf 0L gesetzt ist, wird die Message mit
**          der 1. möglichen DisplayID ausgefüllt.
**          Mit folgendem Konstrukt kann sich also
**          über alle Display-IDs des Objekts informieren:
**
**          struct disp_query_msg dqm;
**          ULONG next_id = 0;
**          do {
**              next_id = _methoda(o, DISPM_Query, &dqm);
**              // hier die Rückgabewerte in dqm auswerten
**          while (dqm.id = next_id);
**
**      DISPM_Begin
**          Msg:    ---
**          Ret:    ---
**
**          Diese Methode kennzeichnet einen neuen Frame.
**          Alle Zeichenoperationen, die für jeweils einen
**          Frame bestimmt sind, müssen in ein 
**          DISPM_Begin/DISPM_End Paar eingeschlossen werden.
**          Aus Effizienz-Gründen dürfen Display-Klassen
**          alle Operationen puffern, oder auf irgendeine
**          andere Art und Weise intern manipulieren. Die
**          Begin/End-Methoden sind dafür eine einfache
**          und universelle Lösung.
**          Bitte beachten, daß Zeichen-Operationen generell in 
**          einen versteckten Puffer gehen, welcher erst bei 
**          DISPM_End sichtbar gemacht wird.
**
**      DISPM_End
**          Msg:    ---
**          Ret:    ---
**
**          Beendet einen Frame. Alle Zeichenoperationen, die
**          seit dem letzten DISPM_Begin auf das Objekt
**          angewendet wurden, werden sichtbar gemacht.
**
**      DISPM_Show
**      DISPM_Hide
**          Msg:    ---
**          Ret:    ---
**
**          Falls eine Treiber-Klasse mehrere gleichzeitig
**          existierende Displays erlaubt, sollte DISPM_Show
**          das Display des Objekts nach vorn bringen.
**          DISPM_Hide sollte das Gegenteil machen.
**
**      DISPM_SetPalette
**          Msg:    struct disp_setpal_msg
**          Ret:    ---
**
**          Setzt einen internen Paletten-Slot auf eine
**          Farbpalette. Die Paletten-Daten werden
**          kopiert. Beachte: ein (S) BMA_ColorMap
**          füllt den Slot #0 aus.
**
**      DISPM_GetPalette
**          Msg:    struct disp_setpal_msg
**          Ret:    ---
**
**          Holt einen Pointer auf einen internen
**          Paletten-Slot.
**
**      DISPM_MixPalette
**          Msg:    struct disp_mixpal_msg
**          Ret:    ---
**
**          Die resultierende (aktuelle) Farbpalette
**          wird entsprechend den Wichtungen in der
**          <disp_mixpal_msg> aus den internen Paletten-
**          Slots berechnet.
**
**      DISPM_SetPointer
**          Msg:    struct disp_pointer_msg
**          Ret:    ---
**
**          Definiert das Aussehen des Mousepointers,
**          NULL schaltet den Mousepointer unsichtbar.
**
**      DISPM_ShowPointer
**      DISPM_HidePointer
**          Msg:    ---
**          Ret:    ---
**
**          Blendet Pointer explizit ein oder aus
**          (bei Direkt-Zugriff auf das Display).
**
**      DISPM_ObtainTexture
**      DISPM_MangleTexture
**      DISPM_ReleaseTexture
**          Msg:    struct disp_texture
**          Ret:    TRUE/FALSE bei DISPM_ObtainTexture, sonst
**                  ---
**
**          Display-Objekte können ein Textur-Format verlangen,
**          welches vom Standard-Format (8bpp/CLUT) abweicht.
**          Zu diesem Zweck bietet die raster.class Texture-
**          Managment-Methoden an, welche von der bitmap.class
**          benutzt werden, sobald ein Bitmap-Object mit
**          dem Attribut BMA_Texture erzeugt wird.
**          Der Ablaufplan sieht dabei so aus:
**
**          (1) Das bitmap.class Object legt eine leere VFMBitmap-
**              Struktur an, und trägt die gewünschte Höhe/Breite
**              ein und setzt die (VBF_Texture) und optional das
**              VBF_HasColorMap Flag.
**          (2) Falls ein "primäres" Display-Objekt existiert (normalerweise
**              der Display-Treiber), wendet das bitmap.class Object
**              die Methode DISPM_ObtainTexture auf dieses Object an.
**              Das raster.class Objekt sollte daraufhin:
**                  * VFMBitmap.Data mit einem Pointer auf die
**                    Pixeldaten initialisieren, die Größe
**                    ist entsprechend (Width*Height*sizeof(Pixel)),
**                    sizeof(Pixel) muß dabei mindestens 1 Byte
**                    betragen!
**                  * VFMBitmap.BytesPerRow ausfüllen.
**                  * optional VFMBitmap.TxtHandle und VFMBitmap.TxtCLUT
**                    initialisieren
**          (3) Das bitmap.class Objekt kann jetzt VFMBitmap.Data
**              und VFMBitmap.ColorMap mit Daten im Standard-Format
**              (8bpp/CLUT, CLUT ist RGB8) ausfüllen und
**              die Methode RASTM_ConvertTexture anwenden.
**              Das raster.class Objekt sollte innerhalb DISPM_MangleTexture
**              die Textur von 8bpp ***In-Place*** in das gewünschte Pixel-
**              Format umwandeln, und optional die TxtCLUT ausfüllen.
**          (4) Damit ist das Erzeugen des Textur-Objects bereits
**              erledigt. Das bitmap.class Objekt wird bei der
**              Freigabe der Textur (wenn das VBF_Texture-Flag gesetzt
**              ist) die Methode DISPM_ReleaseTexture anwenden, dort
**              sollte das raster.class Objekt die Felder
**                  VFMBitmap.Data
**                  VFMBitmap.TxtHandle
**                  VFMBitmap.TxtCLUT ungültig machen
**
**      DISPM_LockTexture
**      DISPM_UnlockTexture
**          Msg:    struct disp_texture
**          Ret:    TRUE/FALSE
**                  ---
**
**          Für Lese-/Schreib-Zugriff auf eine Textur muß diese
**          gelockt werden. Der <Data> Pointer der VFMBitmap
**          Struktur ist (theoretisch...) nur innerhalb eines
**          Lock/Unlock gültig!
**
**      DISPM_BeginSession
**      DISPM_EndSession
**          Msg:    ---
**          Ret:    ---
**
**          Muß vor/nach dem Beginn einer "Session" (z.B.
**          wenn sich das Textur-Set ändert) auf das
**          Display-Treiber-Object angewendet werden.
**
**      DISPM_ScreenShot
**          Msg:    struct disp_screenshot_msg
**          Ret:    TRUE/FALSE
**
**          Erzeugt einen Screen-Shot-File des aktuellen Displays.
**
**  ATTRIBUTES
**
**      DISPA_DisplayID (IG) [ULONG]
**          Beschreibt den grundlegenden Videomodus des
**          Displays, diese Nummer ist device-spezifisch,
**          man kann damit also direkt nichts anfangen. Die Nummer
**          entspricht dem <id> Feld in der <struct disp_query_msg>.
**
**          Per BMA_Width und BMA_Height kann man (wenn
**          das Display-Device es unterstützt) zusätzlich
**          eine X- und Y-Display-Auflösung definieren.
**
**          Falls keine Display-ID angegeben ist, wird die
**          Bitmap nicht angezeigt, sondern fungiert als
**          Hidden-Bitmap.
**
**          Man beachte auch, daß das Attribut nicht settable
**          ist, um einen anderen Display-Modus einzuschalten,
**          muß man also das aktuelle Objekt disposen und
**          ein neues Objekt erzeugen.
**
**      DISPA_DisplayHandle (G) [ULONG]
**          Returniert ein abstraktes Handle. Dieses Handle ist
**          eine spezielle Beschreibung des momentanen Displays,
**          und darf nur an Klassen weitergereicht werden, die
**          wissen, worum's geht! In der Praxis wird dieses
**          Attribut verwendet, um das Input-Handling an das
**          Grafik-Environment zu binden, zum Beispiel für
**          Mouse-Pointer-, oder Input-Focus-Handling.
**
**-----------------------------------------------------------------*/
#define DISPM_BASE      (RASTM_BASE+METHOD_DISTANCE)

#define DISPM_Query             (DISPM_BASE)
#define DISPM_Begin             (DISPM_BASE+1)
#define DISPM_End               (DISPM_BASE+2)
#define DISPM_Show              (DISPM_BASE+3)
#define DISPM_Hide              (DISPM_BASE+4)
#define DISPM_SetPalette        (DISPM_BASE+5)
#define DISPM_MixPalette        (DISPM_BASE+6)
#define DISPM_SetPointer        (DISPM_BASE+7)
#define DISPM_ShowPointer       (DISPM_BASE+8)
#define DISPM_HidePointer       (DISPM_BASE+9)
#define DISPM_ObtainTexture     (DISPM_BASE+10)
#define DISPM_MangleTexture     (DISPM_BASE+11)
#define DISPM_ReleaseTexture    (DISPM_BASE+12)
#define DISPM_LockTexture       (DISPM_BASE+13)
#define DISPM_UnlockTexture     (DISPM_BASE+14)
#define DISPM_BeginSession      (DISPM_BASE+15)
#define DISPM_EndSession        (DISPM_BASE+16)
#define DISPM_GetPalette        (DISPM_BASE+17)
#define DISPM_ScreenShot        (DISPM_BASE+18)

/*-----------------------------------------------------------------*/
#define DISPA_BASE      (RASTA_BASE+ATTRIB_DISTANCE)

#define DISPA_DisplayID     (DISPA_BASE)        /* (IG) */
#define DISPA_DisplayHandle (DISPA_BASE+1)      /* (G) */

/*-----------------------------------------------------------------*/
#define DISPLAY_CLASSID "display.class"

#define DISP_PAL_NUMSLOTS   (8)
#define DISP_PAL_NUMENTRIES (256)

struct disp_RGB {
    UBYTE r,g,b;    // 0..255
};

struct disp_Slot {
    struct disp_RGB rgb[DISP_PAL_NUMENTRIES];
};

struct display_data {
    struct disp_Slot pal;                       // wird angezeigt
    struct disp_Slot slot[DISP_PAL_NUMSLOTS];   // Paletten-Slots
    struct VFMBitmap *pointer;  // Mousepointer-Image
    ULONG flags;                // siehe unten
};

#define DISPF_PointerHidden  (1<<0) // Pointer-Status: sichtbar

/*-----------------------------------------------------------------*/
struct disp_texture {
    struct VFMBitmap *texture;
};

struct disp_query_msg {
    ULONG id;
    ULONG w,h;      // Standard Höhe, Breite
    UBYTE name[32]; // 1 Zeile Beschreibung (Name, Auflösung)
};

struct disp_setpal_msg {
    ULONG slot;     // zu setzender Paletten-Slot
    ULONG first;    // Index des 1. zu ändernden Paletten-Eintrags
    ULONG num;      // Anzahl der Paletten-Einträge
    UBYTE *pal;     // Pointer auf UBYTE[3] Array [R,G,B]
};

struct disp_mixpal_msg {
    ULONG num;      // Anzahl Einträge in den Arrays
    ULONG *slot;    // welche Slots sollen gemischt werden
    ULONG *weight;  // die Wichtungen der einzelnen Slots [0..255]
};

struct disp_pointer_msg {
    struct VFMBitmap *pointer;  // neues Pointer-Image
    ULONG type;                 // siehe unten
};

/*** allgemeiner Pointer-Typ ***/
#define DISP_PTRTYPE_NONE       (0)     // kein Pointer
#define DISP_PTRTYPE_NORMAL     (1)     // Default-Pointer
#define DISP_PTRTYPE_CANCEL     (2)     // Cancel-Pointer
#define DISP_PTRTYPE_SELECT     (3)     // Select-Pointer
#define DISP_PTRTYPE_ATTACK     (4)     // Attack-Pointer
#define DISP_PTRTYPE_GOTO       (5)     // Goto-Pointer
#define DISP_PTRTYPE_DISK       (6)     // Disk-Access (allgemein Wait)
#define DISP_PTRTYPE_NEW        (7)     // New-Pointer
#define DISP_PTRTYPE_ADD        (8)     // Add-Pointer
#define DISP_PTRTYPE_CONTROL    (9)     // Control-Pointer
#define DISP_PTRTYPE_BEAM       (10)    // Beam-Pointer
#define DISP_PTRTYPE_BUILD      (11)    // Build-Pointer

struct disp_screenshot_msg {
    UBYTE *filename;        // Filename des Screenshots
};

/*
** Support: DisplayID-Node. Kann benutzt
** werden, um eine Liste aller 
** verfügbaren Modi aufzubauen.
*/
struct disp_idnode {
    struct MinNode nd;
    ULONG id;           // die "übliche" Display-ID des Modes
    ULONG w,h;          // Default-Höhe und Breite
    UBYTE name[128];    // ein "lesbarer" Name
    ULONG data[8];      // for your own use...
};

/*-----------------------------------------------------------------*/
#endif
