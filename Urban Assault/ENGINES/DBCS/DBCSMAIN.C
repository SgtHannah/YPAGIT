/*
**  $Source$
**  $Revision$
**  $Date$
**  $Locker$
**  $Author$
**
**  dbscmain.c -- Hauptmodul der dbcs.lib.
**
**  (C) Copyright 1997 by A.Weissflog
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DBCS_PADDEDCELL (1)
#include "misc/dbcs.h"

struct dbcs_Handle *DBCS = NULL;

/*-----------------------------------------------------------------*/
void dbcs_KillFont(void)
/*
**  CHANGED
**      20-Nov-97   floh    created
*/
{
    if (DBCS) {
        if (DBCS->hFont) {
            DeleteObject(DBCS->hFont);
            DBCS->hFont = NULL;
        };
    };
} 

/*-----------------------------------------------------------------*/
unsigned long dbcs_SetFont(char *font_def)
/*
**  FUNCTION
**      Erzeugt ein neues Fontobject, entsprechend den Angaben 
**      in "font_def":
**
**          "font_face,height,weigth,charset"
**
**  INPUTS
**      font_def    - String, welcher den Font beschreibt.    
**
**  CHANGED
**      20-Nov-97   floh    created      
*/
{
    if (DBCS) {
        char buf[128];
        char *face_name;
        char *height_str;
        char *weight_str;
        char *chrset_str;
        long height,weight,chr_set;

        strcpy(buf,font_def);
        face_name  = strtok(buf,",");
        height_str = strtok(NULL,",");
        weight_str = strtok(NULL,",");
        chrset_str = strtok(NULL,",");

        if (face_name && height_str && weight_str && chrset_str) {
            height  = atoi(height_str);
            weight  = atoi(weight_str);
            chr_set = atoi(chrset_str);
        } else {
            /*** Char String kaputt ***/
            face_name = "MS Sans Serif";
            height    = 12;
            weight    = 400;
            chr_set   = 0;
        };

        /*** evtl initialisierten Font killen ***/
        dbcs_KillFont();
        DBCS->Height = height;
        DBCS->hFont  = CreateFont(-height,0,0,0,weight,FALSE,FALSE,FALSE,
                                  chr_set, OUT_DEFAULT_PRECIS,
                                  CLIP_DEFAULT_PRECIS,
                                  DRAFT_QUALITY, DEFAULT_PITCH,
                                  face_name);
        if (DBCS->hFont) return(TRUE);
    };
    return(FALSE);
}

/*-----------------------------------------------------------------*/
void dbcs_Kill(void)
/*
**  FUNCTION
**              Raeumt das DBCS-Modul auf.
**
**  CHANGED
**              19-Nov-97       floh    created
*/
{
    if (DBCS) {
        dbcs_KillFont();
        free(DBCS);
        DBCS=NULL;
    };
}

/*-----------------------------------------------------------------*/
unsigned long dbcs_Init(LPDIRECTDRAW lpdd, 
                        LPDIRECTDRAWSURFACE lpsurf,
                        char *font_desc)
/*
**  FUNCTION
**              Initialisiert das DBCS Modul.
**
**  INPUTS
**
**      RESULTS
**              returniert ein Handle, welches bei allen anderen Routinen
**              des DBCS-Moduls uebergeben werden muss. 
**
**  CHANGED
**              19-Nov-97       floh    created
**      20-Nov-97   floh    + Fontdescription String
*/
{
    unsigned long all_ok = FALSE;
    DBCS = malloc(sizeof(struct dbcs_Handle));
    memset(DBCS,0,sizeof(struct dbcs_Handle));
    if (DBCS) {
        DBCS->lpDD   = lpdd;
        DBCS->lpSurf = lpsurf;
        if (dbcs_SetFont(font_desc)) {
            all_ok = TRUE;
        };
    };
    if (!all_ok) dbcs_Kill();
    return(all_ok);
}

/*-----------------------------------------------------------------*/
unsigned long dbcs_BeginText(void)
/*
**  FUNCTION
**              Muss vor einem oder mehreren Aufrufen von dbcs_DrawText()
**              aufgerufen werden. Jedes dbcs_BeginText() muss mit einem
**              dbcs_EndText() korrespondieren.
**              Die Routine unlockt die Backsurface, macht ein
**              GetDC und setzt die benoetigten Text-Parameter.
**      
**      RESULT
**              TRUE    - alles ok
**              FALSE   - Fehler, dbcs_DrawText() NICHT aufrufen!
**
**  CHANGED
**              19-Nov-97       floh    created
**              20-Nov-97       floh    unlockt jetzt vorher die Backsurface
*/
{
    if (DBCS) {
        HRESULT ddrval;
        ddrval = DBCS->lpSurf->lpVtbl->Unlock(DBCS->lpSurf,NULL);
        ddrval = DBCS->lpSurf->lpVtbl->GetDC(DBCS->lpSurf,&(DBCS->hDC));
        if (DD_OK == ddrval) {
            unsigned long rgb = RGB(255,255,0);
            SelectObject(DBCS->hDC,DBCS->hFont);
            SetTextColor(DBCS->hDC,rgb);
            DBCS->ActRGB = rgb;
            SetBkMode(DBCS->hDC,TRANSPARENT);
            return(TRUE);
        };
    };
    return(FALSE);
}

/*-----------------------------------------------------------------*/
unsigned long dbcs_EndText(DDSURFACEDESC *ddsd)
/*
**  FUNCTION
**      Abschluss von dbcs_BeginText()...dbcs_DrawText()...
**
**  INPUTS
**      ddsd    - Pointer auf DDSURFACEDESC Struktur, die beim
**                Lock mit der Surface-Beschreibung ausgefuellt wird
**                (insbesondere der MemPtr und der Pitch!)
**                Die Struktur darf nur ausgewertet werden, wenn die
**                Routine mit TRUE zurueckkommt!
**
**  CHANGED
**      19-Nov-97   floh    created
**      20-Nov-97   floh    nach ReleaseDC wird die Backsurface wieder
**                          gelockt
*/
{
    if (DBCS) {
        HRESULT ddrval;
        if (DBCS->hDC) {
            ddrval = DBCS->lpSurf->lpVtbl->ReleaseDC(DBCS->lpSurf,DBCS->hDC);
            DBCS->hDC = NULL;
            memset(ddsd,0,sizeof(DDSURFACEDESC));
            ddsd->dwSize = sizeof(DDSURFACEDESC);
            ddrval = DBCS->lpSurf->lpVtbl->Lock(DBCS->lpSurf,NULL,ddsd,
                                                DDLOCK_NOSYSLOCK|DDLOCK_WAIT,NULL);
            return(TRUE);
        };
    };
    return(FALSE);
}

/*-----------------------------------------------------------------*/
void dbcs_DrawText(char *text, long x, long y, long w, long h, long flags)
/*
**  FUNCTION
**      Rendert einen Text in die Backsurface. Die Routine darf nur
**      innerhalb dbcs_BeginText() und dbcs_EndText() aufgerufen
**      werden.
**
**  INPUTS
**      dbcs    - DBCS Handle
**      text    - auszugebender Text
**      x,y             - Position des Text
**      w,h             - Ausdehnung des Clipping-Rechtecks
**
**  CHANGED
**      19-Nov-97   floh    created
**      22-Nov-97   floh    + der Text wird jetzt vertikal in der Bounding
**                            Box zentriert
**                          + Alignemnt Handling
**      09-Dec-97   floh    + interpretiert jetzt das Flag DBCSF_COLOR
*/
{
    if (DBCS) {
        if (flags & DBCSF_COLOR) {
            /*** es ist eine Farbdefinition ***/
            unsigned long rgb = RGB(x,y,w);
            if (rgb != DBCS->ActRGB) {
                SetTextColor(DBCS->hDC,rgb);
                DBCS->ActRGB = rgb;
            };
        } else {
            /*** es ist eine normale Textanweisung ***/
            RECT r;
            SIZE sz;

            sz.cx = sz.cy = 0;

            /*** brauchen wir die Breite des Strings? ***/
            if (flags & (DBCSF_RELWIDTH | DBCSF_CENTER | DBCSF_RIGHTALIGN)) {
                GetTextExtentPoint32(DBCS->hDC,text,lstrlen(text),&sz);
            };

            if (flags & DBCSF_RELWIDTH) {
                /*** die Breite ist eine Prozent-Angabe, also umrechnen ***/
                w = (w * sz.cx) / 100;
            };

            /*** das Bounding Rectangle ***/
            r.left   = x;
            r.right  = r.left + w + 1;
            r.top    = y;
            r.bottom = r.top + h + 1;

            /*** Alignment-X ***/
            if (flags & DBCSF_RIGHTALIGN) {
                if (flags & DBCSF_RELWIDTH) {
                    x -= sz.cx;
                    r.left  = x;
                    r.right = r.left + w + 1;
                } else {
                    x += (w - sz.cx);
                };
            } else if (flags & DBCSF_CENTER) {
                if (flags & DBCSF_RELWIDTH) {
                    x -= (sz.cx>>1);
                    r.left  = x;
                    r.right = r.left + w + 1;
                } else {
                    x += ((w - sz.cx)>>1);
                };
            };

            /*** Y zentrieren, Pressed Modus abhandeln ***/
            y += (((h - DBCS->Height)>>1)-1);
            if (flags & DBCSF_PRESSED) {
                x+=1; y+=1;
            };

            /*** String rendern ***/
            SetTextColor(DBCS->hDC,0);
            ExtTextOut(DBCS->hDC,x+1,y+1,ETO_CLIPPED,&r,text,lstrlen(text),NULL);
            SetTextColor(DBCS->hDC,DBCS->ActRGB);
            ExtTextOut(DBCS->hDC,x,y,ETO_CLIPPED,&r,text,lstrlen(text),NULL);
        };
    };
}

/*-----------------------------------------------------------------*/
unsigned long dbcs_Flush(DDSURFACEDESC *ddsd)
/*
**  FUNCTION
**      Gibt alle anstehenden DBCS Strings im Puffer aus.
**      Nach dem Flush wird die uebergebene Surface-Struktur
**      ausgefuellt (MemPtr und Pitch!), ABER NUR, WENN
**      der RETURNVALUE TRUE IST!!!
**      Der Flush muss zwischen RASTM_Begin2D und RASTM_End2D
**      ausgefuehrt werden!
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      20-Nov-97   floh    created
**      25-Nov-97   floh    macht nur noch ein BeginText()/EndText(), wenn
**                          auch tatsaechlich was im DBCS-Puffer steht.
*/
{
    unsigned long i;
    unsigned long retval = FALSE;
    if (DBCS->ActItem > 0) {
        dbcs_BeginText();
        for (i=0; i<DBCS->ActItem; i++) {
            struct dbcs_Item *item = &(DBCS->ItemBuf[i]);
            dbcs_DrawText(item->text,item->x,item->y,item->w,item->h,item->flags);
        };
        DBCS->ActItem = 0;
        retval = dbcs_EndText(ddsd);
    };
    return(retval);
}

/*-----------------------------------------------------------------*/
void dbcs_Add(char *text, long x, long y, long w, long h, unsigned long flags)
/*
**  FUNCTION
**      Traegt einen neuen String in den ItemBuffer ein.
**      Der Puffer wird NICHT automatisch geflusht, wenn
**      er voll ist, die Items werden dann ignoriert.
**      
**  CHANGED
**      20-Nov-97   floh    created
**      25-Nov-97   floh    Align-Code ersetzt durch <flags>
*/
{
    if (DBCS->ActItem < DBCS_MAXNUM_ITEMS) {
        struct dbcs_Item *item = &(DBCS->ItemBuf[DBCS->ActItem++]);
        item->text  = text;
        item->x     = x;
        item->y     = y;
        item->w     = w;
        item->h     = h;
        item->flags = flags;
    };
}

/*-----------------------------------------------------------------*/
char *dbcs_NextChar(char *str)
/*
**  CHANGED
**      02-Dec-97   floh    created
*/
{
    return(CharNext(str));
}

