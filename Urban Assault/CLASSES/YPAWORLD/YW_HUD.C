/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_hud.c,v $
**  $Revision: 38.6 $
**  $Date: 1998/01/06 16:21:26 $
**  $Locker: floh $
**  $Author: floh $
**
**  Head-Up-Display in Vehikeln.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <exec/types.h>

#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "engine/engine.h"
#include "bitmap/displayclass.h"
#include "skeleton/skltclass.h"
#include "ypa/ypagunclass.h"
#include "ypa/ypaworldclass.h"
#include "ypa/guimap.h"

#include "yw_protos.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_ov_engine

extern struct YPAMapReq MR;

#ifdef __WINDOWS__
extern unsigned long wdd_DoDirect3D;
#endif

UBYTE YPA_HudStr[1024];

UBYTE *YPA_HudVecNames[NUM_HUDVEC] = {
    HUDVEC_BAR_NAME,
    HUDVEC_COMPASS_NAME,
    HUDVEC_FRAME_NAME,
    HUDVEC_ARROW_NAME,
    HUDVEC_MG_VISOR_NAME,
    HUDVEC_GRENADE_VISOR_1_NAME,
    HUDVEC_ROCKET_VISOR_1_NAME,
    HUDVEC_MISSILE_VISOR_1_NAME,
    HUDVEC_BOMB_VISOR_1_NAME,
    HUDVEC_GRENADE_VISOR_2_NAME,
    HUDVEC_ROCKET_VISOR_2_NAME,
    HUDVEC_MISSILE_VISOR_2_NAME,
    HUDVEC_BOMB_VISOR_2_NAME,
    HUDVEC_TRIANGLE_NAME,
};

/*-----------------------------------------------------------------*/
void yw_InitHUD(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Initialisiert die Layout-Variablen des HUD.
**
**  CHANGED
**      01-Sep-96   floh    created
**      08-Apr-97   floh    Kompass wird jetzt im unteren Viertel
**                          des gerendert.
**      22-Apr-97   floh    + die HUD-Skeletons werden geladen
**      29-Apr-97   floh    + Visier-Outlines
**      30-Apr-97   floh    + radikal aufgeräumt!
**      14-Dec-97   floh    + Vehicle Information Display Zeugs
*/
{
    struct YPAHud *h = &(ywd->Hud);
    ULONG i;

    /*** löschen... ***/
    memset(h,0,sizeof(struct YPAHud));
    h->RenderHUD = TRUE;

    /*** lade HudVec-Outlines ***/
    _SetAssign("rsrc","data:");
    for (i=0; i<NUM_HUDVEC; i++) {
        h->HudVecObj[i] = _new("sklt.class",RSA_Name,YPA_HudVecNames[i],TAG_DONE);
        if (h->HudVecObj[i]) {
            _get(h->HudVecObj[i], SKLA_Skeleton, &(h->HudVecSklt[i]));
        };
    };

    /*** Vehicle Information Display Sachen ***/
    if (ywd->DspXRes < 512) {
        h->vid_text_h    = (((FLOAT)ywd->Fonts[FONTID_DEFAULT]->height)*1.5) / ((FLOAT)ywd->DspXRes);
        h->vid_numtiles  = 6;
        h->vid_prefix_w  = 18;
        h->vid_postfix_w = 18;
        h->vid_h         = 18 * h->vid_text_h; 
    } else {
        h->vid_text_h    = ((FLOAT)ywd->Fonts[FONTID_DEFAULT]->height) / ((FLOAT)ywd->DspXRes);
        h->vid_numtiles  = 8;
        h->vid_prefix_w  = 28;
        h->vid_postfix_w = 28;
        h->vid_h         = 14 * h->vid_text_h; 
    }; 
    h->vid_w = ((FLOAT)(h->vid_prefix_w + h->vid_postfix_w + 
               ywd->Fonts[FONTID_OWNER_8]->fchars[1].width * h->vid_numtiles)) / 
               ((FLOAT)ywd->DspXRes);

    /*** Hicolor-Flag ***/
    h->is_hicolor = FALSE;
    #ifdef __WINDOWS__
    if (wdd_DoDirect3D) h->is_hicolor = TRUE;
    #endif
}

/*-----------------------------------------------------------------*/
void yw_KillHUD(struct ypaworld_data *ywd)
/*
**  CHANGED
**      22-Apr-97   floh    created
*/
{
    ULONG i;
    struct YPAHud *h = &(ywd->Hud);

    /*** HUD Vektor-Outlines killen ***/
    for (i=0; i<NUM_HUDVEC; i++) {
        if (h->HudVecObj[i]) {
            _dispose(h->HudVecObj[i]);
            h->HudVecObj[i]  = NULL;
            h->HudVecSklt[i] = NULL;
        };
    };
}

/*-----------------------------------------------------------------*/
UBYTE *yw_3DLifebarOverBact(struct ypaworld_data *ywd,
                            UBYTE *str, struct Bacterium *b)
/*
**  FUNCTION
**      Rendert einen Lebensbalken ueber das Bacterium.
**
**  CHANGED
**      13-Dec-97   floh    created
**      22-Apr-98   floh    + jetzt korrekt gerundet.
*/
{
    struct flt_m3x3 *vm   = &(ywd->ViewerDir);
    struct flt_triple *vp = &(ywd->ViewerPos);
    struct flt_triple *p  = &(b->pos);

    FLOAT tx,ty,tz;
    FLOAT x,y,z;

    /*** ermittle x,y im ViewerSpace ***/
    tx = p->x - vp->x;
    ty = p->y - vp->y;
    tz = p->z - vp->z;
    x = vm->m11*tx + vm->m12*ty + vm->m13*tz;
    y = vm->m21*tx + vm->m22*ty + vm->m23*tz;
    z = vm->m31*tx + vm->m32*ty + vm->m33*tz;

    /*** innerhalb des View-Volumes? ***/
    if ((z>30.0) && (x<z) && (x>-z) && (y<z) && (y>-z)) {

        ULONG fnt_id = FONTID_OWNER_4;
        struct VFMFont *fnt = ywd->Fonts[fnt_id];
        WORD xpos,ypos,x_size,y_size;
        LONG num_chars,energy_per_char,energy,maximum,act;
        LONG clip_x0,clip_y0,clip_x1,clip_y1;
        BOOL do_map_clip;

        /*** Clip-Parameter gegen Map ***/
        clip_x0 = clip_x1 = 0;
        clip_y0 = clip_y1 = 0;
        if (!(MR.req.flags & REQF_Closed)) {
            do_map_clip = TRUE;
            clip_x0 = MR.req.req_cbox.rect.x;
            clip_x1 = clip_x0 + MR.req.req_cbox.rect.w;
            clip_y0 = MR.req.req_cbox.rect.y;
            clip_y1 = clip_y0 + MR.req.req_cbox.rect.h;
        }; 

        /*** 2D transformieren ***/
        x /= z;
        y /= z;

        /*** String-Position und Groesse ***/
        if (ywd->DspXRes < 512) num_chars = 4;
        else                    num_chars = 8;
        x_size = fnt->fchars[0].width * num_chars;
        y_size = ywd->Fonts[fnt_id]->height;
        xpos = (ywd->DspXRes>>1) * (x + 1.0);
        ypos = (ywd->DspYRes>>1) * (y + 1.0);
        if (do_map_clip&&(xpos>clip_x0)&&(xpos<clip_x1)&&(ypos>clip_y0)&&(ypos<clip_y1)) {
            /*** Abbruch ***/
            return(str);
        };
        xpos -= (x_size>>1);
        ypos -= (y_size>>1);

        /*** ypos noch etwas nach oben versetzen ***/
        ypos -= (ywd->DspYRes/16);

        /*** 2D-Clipping ***/
        if ((xpos < 0) || ((xpos+x_size)>=ywd->DspXRes) ||
            (ypos < 0) || ((ypos+y_size)>=ywd->DspYRes))
        {
            return(str);
        };
        xpos -= (ywd->DspXRes>>1);
        ypos -= (ywd->DspYRes>>1);

        new_font(str,fnt_id);
        pos_abs(str,xpos,ypos);

        /*** Lebensbalken rendern ***/
        energy_per_char = b->Maximum / num_chars;
        energy  = b->Energy;
        maximum = b->Maximum;
        for (act=1; act<=num_chars; act++) {
            LONG rounded_energy = (act * energy_per_char) - (energy_per_char>>1);        
            if (rounded_energy <= energy) {
                put(str,2); // gruen
            } else {
                put(str,6); // rot
            };
        };
    };
    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_3DNameOverBact(struct ypaworld_data *ywd, 
                         UBYTE *str,
                         struct Bacterium *b)
/*
**  FUNCTION
**      Rendert den Multiplayer-Namen ueber ein Vehikel.
**
**  CHANGED
**      21-May-98   floh    created
*/
{
    if (ywd->gsr && ywd->playing_network) {
    
        struct flt_m3x3 *vm   = &(ywd->ViewerDir);
        struct flt_triple *vp = &(ywd->ViewerPos);
        struct flt_triple *p  = &(b->pos);
        UBYTE *name = ywd->gsr->player[b->Owner].name;
        FLOAT tx,ty,tz;
        FLOAT x,y,z;
        
        /*** Name ueberhaupt gueltig? ***/        
        if (!name[0]) return(str);        
        
        /*** ermittle x,y im ViewerSpace ***/
        tx = p->x - vp->x;
        ty = p->y - vp->y;
        tz = p->z - vp->z;
        x = vm->m11*tx + vm->m12*ty + vm->m13*tz;
        y = vm->m21*tx + vm->m22*ty + vm->m23*tz;
        z = vm->m31*tx + vm->m32*ty + vm->m33*tz;

        /*** innerhalb des View-Volumes? ***/
        if ((z>30.0) && (x<z) && (x>-z) && (y<z) && (y>-z)) {

            ULONG fnt_id = FONTID_TRACY;
            struct VFMFont *fnt = ywd->Fonts[fnt_id];
            WORD xpos,ypos,x_size,y_size;
            ULONG color_index;

            /*** 2D transformieren ***/
            x /= z;
            y /= z;

            /*** String-Position und Groesse ***/
            x_size = 96;
            y_size = fnt->height;
            xpos = (ywd->DspXRes>>1) * (x + 1.0);
            ypos = (ywd->DspYRes>>1) * (y + 1.0);
            if ((xpos+x_size) >= ywd->DspXRes) x_size = ywd->DspXRes - xpos - 1;

            /*** 2D-Clipping ***/
            if (x_size<=0) return(str);
            if ((xpos<0)||((xpos+x_size)>=ywd->DspXRes)||(ypos<0)||((ypos+y_size)>=ywd->DspYRes))
            {
                return(str);
            };
            xpos -= (ywd->DspXRes>>1);
            ypos -= (ywd->DspYRes>>1);

            new_font(str,fnt_id);
            pos_abs(str,xpos,ypos);

            /*** Namen rendern ***/
            color_index = YPACOLOR_OWNER_0 + b->Owner;  
            dbcs_color(str,yw_Red(ywd,color_index),yw_Green(ywd,color_index),yw_Blue(ywd,color_index));
            str = yw_TextBuildClippedItem(fnt,str,name,x_size,' '); 
        };
    };
    return(str);
}

/*-----------------------------------------------------------------*/
void yw_VectorOutline(struct ypaworld_data *ywd,
                      struct Skeleton *sklt,
                      FLOAT tx,  FLOAT ty,
                      FLOAT m11, FLOAT m12,
                      FLOAT m21, FLOAT m22,
                      FLOAT sx, FLOAT sy,
                      ULONG color,
                      void (*local_color_hook)(struct ypaworld_data *ywd, FLOAT x0, FLOAT y0, FLOAT x1, FLOAT y1, ULONG *c0, ULONG *c1),
                      void (*global_color_hook)(struct ypaworld_data *ywd, FLOAT x0, FLOAT y0, FLOAT x1, FLOAT y1, ULONG *c0, ULONG *c1))
/*
**  FUNCTION
**      Rendert eine 2D-Outline, definiert durch ein Skeleton
**      mit 2-Punkt-"Polygonen".
**      Der Punkt wird zuerst rotiert/skaliert (über die Matrix),
**      dann translatiert.
**      Die Farbe wird im Hicolor-Modus als RGB-Triplet übergeben,
**      sonst als CLUT-Index.
**
**  CHANGED
**      22-Apr-97   floh    created
**      31-Jul-97   floh    + ColorModifier-Hooks
*/
{
    if (sklt) {

        #define UNIT_SIZE (1000.0)
        ULONG i;
        struct rast_pens rp;

        /*** setze Vordergrund-Farbe ***/
        rp.fg_pen  = color;
        rp.fg_apen = color;
        rp.bg_pen  = -1;
        _methoda(ywd->GfxObject, RASTM_SetPens, &rp);

        /*** Punkt-Pool transformieren... ***/
        for (i=0; i<sklt->NumPoolPoints; i++) {
            FLOAT x0,y0,x1,y1;
            x0 = (sklt->Pool[i].x / UNIT_SIZE);
            y0 = (-sklt->Pool[i].z / UNIT_SIZE);
            x1 = (x0*m11 + y0*m12) * sx;
            y1 = (x0*m21 + y0*m22) * sy;
            sklt->TransformPool[i].x = x1 + tx;
            sklt->TransformPool[i].y = y1 + ty;
        };

        /*** alle Polygone durchackern und als Linie rendern (die ***/
        /*** ersten zwei Punkte werden gerendert)                 ***/
        for (i=0; i<sklt->NumAreas; i++) {
            if (sklt->Areas[i][0] >= 2) {
                struct rast_line rl;
                ULONG i0 = sklt->Areas[i][1];
                ULONG i1 = sklt->Areas[i][2];
                rl.x0 = sklt->TransformPool[i0].x;
                rl.y0 = sklt->TransformPool[i0].y;
                rl.x1 = sklt->TransformPool[i1].x;
                rl.y1 = sklt->TransformPool[i1].y;
                if (local_color_hook) {
                    ULONG c0 = color;
                    ULONG c1 = color;
                    local_color_hook(ywd, (FLOAT)(rl.x0-tx), (FLOAT)(rl.y0-ty),
                                          (FLOAT)(rl.x1-tx), (FLOAT)(rl.y1-ty),
                                          &c0, &c1);
                    rp.fg_pen  = c0;
                    rp.fg_apen = c1;
                    rp.bg_pen  = -1;
                    _methoda(ywd->GfxObject, RASTM_SetPens, &rp);
                } else if (global_color_hook) {
                    ULONG c0 = color;
                    ULONG c1 = color;
                    global_color_hook(ywd, rl.x0, rl.y0, rl.x1, rl.y1, &c0, &c1);
                    rp.fg_pen  = c0;
                    rp.fg_apen = c1;
                    rp.bg_pen  = -1;
                    _methoda(ywd->GfxObject, RASTM_SetPens, &rp);
                };
                _methoda(ywd->GfxObject,RASTM_ClippedLine,&rl);
            };
        };
    };
}

/*-----------------------------------------------------------------*/
void yw_3DCursorOverBact(struct ypaworld_data *ywd, struct Bacterium *b)
/*
**  FUNCTION
**      Rendert Char ueber Vehikel im 3D-View.
**
**  INPUTS
**      ywd
**      str     - Output-Stream
**      b       - Bakterium, ueber welchem das Char gerendert wird
**      fnt     - Font-Nummer des Chars
**      chr     - das Char itself
**      size    - Groesse des Char in Pixel
**
**  CHANGED
**      29-Jun-96   floh    created
**      22-Jul-96   floh    kommt jetzt mit ExtraViewer-Zeuch
**                          klar
**      01-Sep-96   floh    uebernommen nach yw_hud.c
**      17-Dec-97   floh    umgeschrieben auf Vektor-Outline
**      14-Jan-98   floh    kleinere Pfeile, ausgewähltes Geschwader
**                          wird nicht mehr per Viereck angezeigt.
*/
{
    struct flt_m3x3 *vm   = &(ywd->ViewerDir);
    struct flt_triple *vp = &(ywd->ViewerPos);
    struct flt_triple *p  = &(b->pos);

    FLOAT tx,ty,tz;
    FLOAT x,y,z;

    /*** ermittle x,y im ViewerSpace ***/
    tx = p->x - vp->x;
    ty = p->y - vp->y;
    tz = p->z - vp->z;
    x = vm->m11*tx + vm->m12*ty + vm->m13*tz;
    y = vm->m21*tx + vm->m22*ty + vm->m23*tz;
    z = vm->m31*tx + vm->m32*ty + vm->m33*tz;

    /*** innerhalb des View-Volumes? ***/
    if ((z>30.0) && (x<z) && (x>-z) && (y<z) && (y>-z)) {

        struct Skeleton *sklt;
        struct YPAHud *h = &(ywd->Hud);
        BOOL is_selected = FALSE;
        ULONG color = yw_GetColor(ywd,YPACOLOR_OWNER_0 + b->Owner); 

        /*** 2D transformieren ***/
        x /= z;
        y /= z;

        /*** Vektor-Outline rendern ***/
        sklt = h->HudVecSklt[HUDVEC_TRIANGLE];
        if (sklt) {
            FLOAT m11,m12,m21,m22,sx,sy;
            m11 = 1.0;  m12 = 0.0;
            m21 = 0.0;  m22 = 1.0;
            
            /*** Commander bekommt einen doppelten ***/
            if ((b->master == b->robo) && (b->Owner == ywd->URBact->Owner)) {
                sx = 0.015;
                sy = 0.02;
                yw_VectorOutline(ywd,sklt,x,y-0.08,m11,m12,m21,m22,sx,sy,color,NULL,NULL);
                sx = 0.005;
                sy = 0.00666;
                yw_VectorOutline(ywd,sklt,x,y-0.08,m11,m12,m21,m22,sx,sy,color,NULL,NULL);
            } else {
                sx = 0.0075;
                sy = 0.01;
                yw_VectorOutline(ywd,sklt,x,y-0.08,m11,m12,m21,m22,sx,sy,color,NULL,NULL);
            };
        };
        if (is_selected) {
            sklt = h->HudVecSklt[HUDVEC_FRAME];
            if (sklt) {
                FLOAT m11,m12,m21,m22,sx,sy;
                sx = 0.0075;
                sy = 0.01;
                m11 = 1.0;  m12 = 0.0;
                m21 = 0.0;  m22 = 1.0;
                yw_VectorOutline(ywd,sklt,x,y,m11,m12,m21,m22,sx,sy,color,NULL,NULL);
            };
        };
    };
}

/*-----------------------------------------------------------------*/
void yw_SetInterpolateColors(struct ypaworld_data *ywd, 
                             ULONG rgb0, ULONG rgb1)
/*
**  FUNCTION
**      Setzt die von yw_InterpolateColor() verwendeten Basis-
**      farben.      
**
**  CHANGED
**      16-Dec-97   floh    created
*/
{
    struct YPAHud *h = &(ywd->Hud);
    h->r0 = (FLOAT) ((rgb0>>16) & 0xff);
    h->g0 = (FLOAT) ((rgb0>>8)  & 0xff);
    h->b0 = (FLOAT) ((rgb0)     & 0xff);
    h->r1 = (FLOAT) ((rgb1>>16) & 0xff);
    h->g1 = (FLOAT) ((rgb1>>8)  & 0xff);
    h->b1 = (FLOAT) ((rgb1)     & 0xff);
}

/*-----------------------------------------------------------------*/
ULONG yw_InterpolateColors(struct ypaworld_data *ywd, FLOAT d)
/*
**  FUNCTION
**      Rueckgabewert ist mit d interpolierte Farbe der per 
**      yw_SetInterpolateColor() gesetzten Basis-Farben. 
**
**  CHANGED
**      16-Dec-97   floh    created
*/
{
    struct YPAHud *h = &(ywd->Hud);
    ULONG r,g,b;
    r = ((ULONG)(h->r0 + ((h->r1 - h->r0)*d))) << 16;
    g = ((ULONG)(h->g0 + ((h->g1 - h->g0)*d))) << 8;
    b = ((ULONG)(h->b0 + ((h->b1 - h->b0)*d)));
    return(r|g|b);
}

/*-----------------------------------------------------------------*/
void yw_CompassColorHook(struct ypaworld_data *ywd,
                         FLOAT x0, FLOAT y0, FLOAT x1, FLOAT y1,
                         ULONG *pc0, ULONG *pc1)
/*
**  FUNCTION
**      Modifiziert die Linien-Farben des Radars so, dass ein
**      "umlaufender Helligkeits-Schweif-Effekt" entsteht.
**
**  CHANGED
**      31-Jul-97   floh    created
**      16-Dec-97   floh    umgeschrieben fuer yw_InterpolateColors() 
*/
{
    struct YPAHud *h = &(ywd->Hud);
    FLOAT rx,ry;
    FLOAT l;
    FLOAT sp0, sp1;
    FLOAT d0, d1;

    /*** aktuelle Radar-Position auf Vollkreis (ein Umlauf pro Sekunde) ***/
    FLOAT rad = h->breath_pos * 6.28;

    /*** daraus einen 2D-Vektor ***/
    rx = cos(rad);
    ry = sin(rad);

    /*** Start- und Endpunkt als normierte Vektoren ***/
    l = nc_sqrt(x0*x0 + y0*y0);
    if (l > 0.0) {
        x0/=l; y0/=l;
    };
    l = nc_sqrt(x1*x1 + y1*y1);
    if (l > 0.0) {
        x1/=l; y1/=l;
    };

    /*** Skalar-Produkte (liegt dann irgendwo zwischen -1.0 und +1.0 ***/
    sp0 = rx*x0 + ry*y0;
    sp1 = rx*x1 + ry*y1;
    d0  = (sp0 + 1.0) * 0.5;
    d1  = (sp1 + 1.0) * 0.5;
    *pc0 = yw_InterpolateColors(ywd,d0);
    *pc1 = yw_InterpolateColors(ywd,d1);
}

/*-----------------------------------------------------------------*/
void yw_SlowPulsateColorHook(struct ypaworld_data *ywd,
                             FLOAT x0, FLOAT y0, FLOAT x1, FLOAT y1,
                             ULONG *pc0, ULONG *pc1)
/*
**  CHANGED
**      31-Jul-97   floh    created
**      16-Dec-97   floh    umgeschrieben
*/
{
    struct YPAHud *h = &(ywd->Hud);
    FLOAT dx0,dx1;
    ULONG r0,g0,b0,r1,g1,b1;
    ULONG c0 = *pc0;
    ULONG c1 = *pc1;
    FLOAT h0, h1;
    FLOAT s = (h->breath_pos * 2.0) - 1.0;

    /*** s -> Scanposition, -0.5 .. +1.5         ***/
    /*** dx0, dx1 -> Entfernung zur Scanposition ***/
    dx0 = fabs(s - nc_sqrt(x0*x0 + y0*y0));
    dx1 = fabs(s - nc_sqrt(x1*x1 + y1*y1));
    if (dx0<0.0)        dx0=0.0;
    else if (dx0 > 1.0) dx0=1.0;
    if (dx1<0.0)        dx1=0.0;
    else if (dx1 > 1.0) dx1=1.0;
    *pc0 = yw_InterpolateColors(ywd,dx0);
    *pc1 = yw_InterpolateColors(ywd,dx1);
}

/*-----------------------------------------------------------------*/
void yw_FastPulsateColorHook(struct ypaworld_data *ywd,
                             FLOAT x0, FLOAT y0, FLOAT x1, FLOAT y1,
                             ULONG *pc0, ULONG *pc1)
/*
**  CHANGED
**      31-Jul-97   floh    created
*/
{
    FLOAT h;
    FLOAT r = (((FLOAT)(ywd->TimeStamp % 300)) / 300.0);
    h = r;
    *pc0 = yw_InterpolateColors(ywd,h);
    *pc1 = yw_InterpolateColors(ywd,h);
}

/*-----------------------------------------------------------------*/
void yw_L2RScanColorHook(struct ypaworld_data *ywd,
                         FLOAT x0, FLOAT y0, FLOAT x1, FLOAT y1,
                         ULONG *pc0, ULONG *pc1)
/*
**  CHANGED
**      31-Jul-97   floh    created
*/
{
    struct YPAHud *h = &(ywd->Hud);
    FLOAT dx0,dx1;
    FLOAT s = (h->breath_pos*2.0)-1.5;

    /*** s -> Scanposition, -1.0 .. +1.0         ***/
    /*** dx0, dx1 -> Entfernung zur Scanposition ***/
    dx0 = fabs(s - x0);
    dx1 = fabs(s - x1);
    if (dx0 > 0.5) dx0=0.5;
    if (dx1 > 0.5) dx1=0.5;
    dx0 *= 2.0;
    dx1 *= 2.0;
    *pc0 = yw_InterpolateColors(ywd,dx0);
    *pc1 = yw_InterpolateColors(ywd,dx1);
}

/*-----------------------------------------------------------------*/
void yw_R2LScanColorHook(struct ypaworld_data *ywd,
                         FLOAT x0, FLOAT y0, FLOAT x1, FLOAT y1,
                         ULONG *pc0, ULONG *pc1)
/*
**  CHANGED
**      31-Jul-97   floh    created
*/
{
    struct YPAHud *h = &(ywd->Hud);
    FLOAT dx0,dx1;
    FLOAT s = (h->breath_pos*(-2.0)) + 1.5;

    /*** s -> Scanposition, -1.0 .. +1.0         ***/
    /*** dx0, dx1 -> Entfernung zur Scanposition ***/
    dx0 = fabs(s - x0);
    dx1 = fabs(s - x1);
    if (dx0 > 0.5) dx0=0.5;
    if (dx1 > 0.5) dx1=0.5;
    dx0 *= 2.0;
    dx1 *= 2.0;
    *pc0 = yw_InterpolateColors(ywd,dx0);
    *pc1 = yw_InterpolateColors(ywd,dx1);
}

/*-----------------------------------------------------------------*/
void yw_VecRenderCompass(struct ypaworld_data *ywd, struct YPAHud *h)
/*
**  CHANGED
**      23-Apr-97   floh    created
**      17-May-97   floh    + RGB-Farben redefiniert
**      09-Jun-97   floh    + etwas nach oben verlagert
**      03-Jul-97   floh    + Sektor-Owner-Farben fuer HiColor angepasst
**      09-Dec-97   floh    + yw_GetColor()
*/
{
    FLOAT tx,ty,sx,sy,m11,m12,m21,m22;
    FLOAT r_m11,r_m12,r_m21,r_m22;
    FLOAT r_x,r_y;
    FLOAT l;
    ULONG color0,color1;
    FLOAT dt = (ywd->TimeStamp - h->change_vhcl_timer)/180.0;
    struct Bacterium *b = ywd->UVBact;
    void (*render_hook)(struct ypaworld_data *,FLOAT,FLOAT,
        FLOAT,FLOAT,ULONG *,ULONG *) = h->is_hicolor ? yw_CompassColorHook:NULL;

    /*** Mittelpunkt und Ausrichtung des Kompass ***/
    tx = 0.7;
    ty = 0.4 - 0.1;
    m21 = ywd->ViewerDir.m13;
    m22 = ywd->ViewerDir.m33;
    l = nc_sqrt(m21*m21 + m22*m22);
    if (l > 0.0) {
        m21/=l; m22/=l;
    };
    m11 = m22;
    m12 = -m21;

    /*** Richtung zum Vorgesetzten ***/
    sx = 0.25;
    sy = 0.3;
    color0 = yw_GetColor(ywd,YPACOLOR_HUD_COMPASS_COMMANDVEC_0);
    color1 = yw_GetColor(ywd,YPACOLOR_HUD_COMPASS_COMMANDVEC_1);
    r_x = b->master_bact->pos.x - b->pos.x;
    r_y = b->master_bact->pos.z - b->pos.z;
    r_m21 = r_x*m11 + r_y*m21;
    r_m22 = r_x*m12 + r_y*m22;
    l = nc_sqrt(r_m21*r_m21 + r_m22*r_m22);
    if (l > 30.0) {
        r_m21 /= l; r_m22 /= l;
        r_m11 = r_m22;
        r_m12 = -r_m21;
        yw_SetInterpolateColors(ywd,color0,color1);
        yw_VectorOutline(ywd,h->HudVecSklt[HUDVEC_ARROW],
                         tx, ty,
                         r_m11, r_m12, r_m21, r_m22,
                         sx, sy, color0, render_hook, NULL);
    };

    /*** falls Geschwaderführer, Richtung zum Primtarget ***/
    if (b->master == b->robo) {
        BOOL p_ok = FALSE;
        if (TARTYPE_SECTOR == ywd->UVBact->PrimTargetType) {
            r_x = b->PrimPos.x - b->pos.x;
            r_y = b->PrimPos.z - b->pos.z;
            p_ok = TRUE;
        } else if (TARTYPE_BACTERIUM == b->PrimTargetType) {
            r_x = b->PrimaryTarget.Bact->pos.x;
            r_y = b->PrimaryTarget.Bact->pos.z;
            p_ok = TRUE;
        };
        if (p_ok) {
            color0 = yw_GetColor(ywd,YPACOLOR_HUD_COMPASS_PRIMTARGET_0);
            color1 = yw_GetColor(ywd,YPACOLOR_HUD_COMPASS_PRIMTARGET_1);
            r_m21 = r_x*m11 + r_y*m21;
            r_m22 = r_x*m12 + r_y*m22;
            l = nc_sqrt(r_m21*r_m21 + r_m22*r_m22);
            if (l > 30.0) {
                r_m21 /= l; r_m22 /= l;
                r_m11 = r_m22;
                r_m12 = -r_m21;
                yw_SetInterpolateColors(ywd,color0,color1);
                yw_VectorOutline(ywd,h->HudVecSklt[HUDVEC_ARROW],
                                 tx, ty,
                                 r_m11, r_m12, r_m21, r_m22,
                                 sx, sy, color0, render_hook, NULL);
            };
        };
    };

    /*** falls aufgelockt, Richtung zum Ziel ***/
    if (ywd->visor.target) {
        r_x = ywd->visor.target->pos.x - b->pos.x;
        r_y = ywd->visor.target->pos.z - b->pos.z;
        color0 = yw_GetColor(ywd,YPACOLOR_HUD_COMPASS_LOCKTARGET_0);
        color1 = yw_GetColor(ywd,YPACOLOR_HUD_COMPASS_LOCKTARGET_1);
        r_m21 = r_x*m11 + r_y*m21;
        r_m22 = r_x*m12 + r_y*m22;
        l = nc_sqrt(r_m21*r_m21 + r_m22*r_m22);
        if (l > 30.0) {
            r_m21 /= l; r_m22 /= l;
            r_m11 = r_m22;
            r_m12 = -r_m21;
            yw_SetInterpolateColors(ywd,color0,color1);
            yw_VectorOutline(ywd,h->HudVecSklt[HUDVEC_ARROW],
                             tx, ty,
                             r_m11, r_m12, r_m21, r_m22,
                             sx, sy, color0, render_hook, NULL);
        };
    };

    /*** der Kompass itself ***/
    color0 = yw_GetColor(ywd,YPACOLOR_HUD_COMPASS_COMPASS_0);
    color1 = yw_GetColor(ywd,YPACOLOR_HUD_COMPASS_COMPASS_1);
    sx = 0.25;
    sy = 0.3;
    if (dt < 1.4) { sx*=dt; sy*=dt; };
    yw_SetInterpolateColors(ywd,color0,color1);
    yw_VectorOutline(ywd,h->HudVecSklt[HUDVEC_COMPASS],
                     tx, ty,
                     m11, m12, m21, m22,
                     sx, sy, color0, render_hook, NULL);

    /*** die Sektor-Owner-Anzeige ***/
    switch(b->Sector->Owner) {
        case 0:
            color0 = yw_GetColor(ywd,YPACOLOR_OWNER_0);
            break;
        case 1:
            color0 = yw_GetColor(ywd,YPACOLOR_OWNER_1);
            break;
        case 2:
            color0 = yw_GetColor(ywd,YPACOLOR_OWNER_2);
            break;
        case 3:
            color0 = yw_GetColor(ywd,YPACOLOR_OWNER_3);
            break;
        case 4:
            color0 = yw_GetColor(ywd,YPACOLOR_OWNER_4);
            break;
        case 5:
            color0 = yw_GetColor(ywd,YPACOLOR_OWNER_5);
            break;
        case 6:
            color0 = yw_GetColor(ywd,YPACOLOR_OWNER_6);
            break;
        case 7:
            color0 = yw_GetColor(ywd,YPACOLOR_OWNER_7);
            break;
    };
    sx = 0.07;
    sy = 0.08;
    yw_SetInterpolateColors(ywd,color0,color0);
    yw_VectorOutline(ywd,h->HudVecSklt[HUDVEC_FRAME],
                     tx, ty,
                     m11, m12, m21, m22,
                     sx, sy, color0, render_hook, NULL);
}

/*-----------------------------------------------------------------*/
void yw_VecRenderVehicle(struct ypaworld_data *ywd, 
                         struct YPAHud *h,
                         struct VehicleProto *vp,
                         FLOAT tx, FLOAT ty, FLOAT dt)
/*
**  CHANGED
**      23-Apr-97   floh    created
**      17-May-97   floh    + RGB-Farben redefiniert
**      09-Jun-97   floh    + etwas nach oben verlagert
**      09-Dec-97   floh    + yw_GetColor()
**      14-Dec-97   floh    + umgeschrieben fuer yw_RenderVID()
*/
{
    FLOAT sx,sy,m11,m12,m21,m22;
    ULONG color1 = yw_GetColor(ywd,YPACOLOR_HUD_VEHICLE_0);
    ULONG color0 = yw_GetColor(ywd,YPACOLOR_HUD_VEHICLE_1);
    struct Skeleton *sklt = NULL;
    void (*render_hook)(struct ypaworld_data *,FLOAT,FLOAT,
        FLOAT,FLOAT,ULONG *,ULONG *) = h->is_hicolor ? yw_L2RScanColorHook:NULL;

    /*** trage Vehikel-Wireframe-Skeleton ein ***/
    if (vp->wireframe_object) _get(vp->wireframe_object,SKLA_Skeleton,&sklt);
    if (sklt) {
        // sx =  0.25;
        // sy =  0.3;
        
        sx = 0.125 * 0.8;        
        sy = 6 * h->vid_text_h * 0.8;        
        if (dt < 1.4) sx*=dt;
        m11 = 1.0;  m12 = 0.0;
        m21 = 0.0;  m22 = 1.0;
        yw_SetInterpolateColors(ywd,color0,color1);
        yw_VectorOutline(ywd,sklt,tx,ty,m11,m12,m21,m22,sx,sy,color0,NULL,render_hook);
    };
}

/*-----------------------------------------------------------------*/
void yw_VecRenderWeapon(struct ypaworld_data *ywd, 
                        struct YPAHud *h, 
                        struct WeaponProto *wp,
                        FLOAT tx, FLOAT ty)
/*
**  FUNCTION
**      Rendert den Weapon-Reload-Bar.
**
**  CHANGED
**      28-Apr-97   floh    created
**      09-Jun-97   floh    etwas nach oben verschoben
*/
{
    struct Skeleton *sklt = NULL;
    void (*render_hook)(struct ypaworld_data *,FLOAT,FLOAT,
        FLOAT,FLOAT,ULONG *,ULONG *) = h->is_hicolor ? yw_L2RScanColorHook:NULL;

    if (wp) {
        /*** hole Weapon-Wireframe-Skeleton ***/
        if (wp->wireframe_object) _get(wp->wireframe_object,SKLA_Skeleton,&sklt);
        if (sklt) {
            FLOAT sx,sy,m11,m12,m21,m22;
            ULONG color1 = yw_GetColor(ywd,YPACOLOR_HUD_WEAPON_0);
            ULONG color0 = yw_GetColor(ywd,YPACOLOR_HUD_WEAPON_1);    

            /*** im rechten oberen Drittel des Vehikel-Wireframes ***/
            // sx =  0.083;
            // sy =  0.1;
            sx = 0.0415;
            sy = 0.05;
            m11 = 1.0; m12 = 0.0;
            m21 = 0.0; m22 = 1.0;
            yw_SetInterpolateColors(ywd,color0,color1);
            yw_VectorOutline(ywd,sklt,tx,ty,m11,m12,m21,m22,sx,sy,color0,NULL,render_hook);
        };
    };
}

/*-----------------------------------------------------------------*/
UBYTE *yw_RenderTileBar(struct ypaworld_data *ywd,
                        struct YPAHud *h,
                        UBYTE *str,
                        FLOAT x, FLOAT y,
                        LONG filled, LONG maximum,
                        UBYTE fill_chr, UBYTE rest_chr,
                        UBYTE *prefix, UBYTE *postfix)
/*
**  FUNCTION
**      Rendert einen Fuellbalken fuer das Vehikel-Information-
**      Display. Die Position definiert den Mittelpunkt des
**      ganzen. Fuer Pre- und Postfix sind je 24 bzw. 12 Pixel 
**      reserviert.
**
**  CHANGED
**      15-Dec-97   floh    created
**      16-Dec-97   floh    + Clipping gegen Map
**      03-Apr-98   floh    +
*/
{
    WORD xpos,ypos;
    WORD all_w, bar_w, tile_w, tile_h;
    LONG filled_per_char,act;
    LONG clip_x0,clip_y0,clip_x1,clip_y1;
    LONG mx,my;
    BOOL do_map_clip = FALSE;

    /*** falls Map offen, gegen Map clippen ***/
    clip_x0 = clip_x1 = 0;
    clip_y0 = clip_y1 = 0;
    if (!(MR.req.flags & REQF_Closed)) {
        do_map_clip = TRUE;
        clip_x0 = MR.req.req_cbox.rect.x - (ywd->DspXRes>>1);
        clip_x1 = clip_x0 + MR.req.req_cbox.rect.w;
        clip_y0 = MR.req.req_cbox.rect.y - (ywd->DspYRes>>1);
        clip_y1 = clip_y0 + MR.req.req_cbox.rect.h;
    }; 

    xpos = x * (ywd->DspXRes>>1);
    ypos = y * (ywd->DspYRes>>1);

    bar_w = h->vid_numtiles * ywd->Fonts[FONTID_OWNER_8]->fchars[fill_chr].width;
    all_w = bar_w + h->vid_prefix_w + h->vid_postfix_w;
    xpos -= (all_w>>1);

    /*** Prefix String ***/
    dbcs_color(str,yw_Red(ywd,YPACOLOR_TEXT_HUD),yw_Green(ywd,YPACOLOR_TEXT_HUD),yw_Blue(ywd,YPACOLOR_TEXT_HUD));
    if (prefix) {
        mx = xpos + (h->vid_prefix_w>>1);
        my = ypos;
        if (!(do_map_clip && (mx>clip_x0) && (mx<clip_x1) && (my>clip_y0) && (my<clip_y1))) {
            new_font(str,FONTID_TRACY);
            pos_abs(str,xpos,(ypos-(ywd->FontH>>1)));
            str = yw_TextRelWidthItem(ywd->Fonts[FONTID_TRACY],str,prefix,100,YPACOLF_ALIGNLEFT);
        };
    };

    /*** Fuell-String ***/
    xpos += h->vid_prefix_w;
    new_font(str,FONTID_OWNER_8);
    tile_w = ywd->Fonts[FONTID_OWNER_8]->fchars[1].width;
    tile_h = ywd->Fonts[FONTID_OWNER_8]->height;
    mx = xpos + (tile_w>>1);
    my = ypos + (tile_h>>1); 
    pos_abs(str,xpos,(ypos-(tile_h>>1)));
    filled_per_char = maximum / h->vid_numtiles;
    for (act=1; act<=h->vid_numtiles; act++) {
        if (!(do_map_clip && (mx>clip_x0) && (mx<clip_x1) && (my>clip_y0) && (my<clip_y1))) {
            LONG rounded_filled = (act*filled_per_char)-(filled_per_char>>1);
            if (rounded_filled <= filled) {
                put(str,fill_chr);
            } else {
                put(str,rest_chr);
            };
        };
        mx += tile_w;
    };

    /*** Postfix String ***/
    xpos += (bar_w + h->vid_postfix_w);
    if (postfix) {
        mx = xpos - (h->vid_postfix_w>>1);
        my = ypos;
        if (!(do_map_clip && (mx>clip_x0) && (mx<clip_x1) && (my>clip_y0) && (my<clip_y1))) {
            new_font(str,FONTID_TRACY);
            pos_abs(str,xpos,(ypos-(ywd->FontH>>1)));
            str = yw_TextRelWidthItem(ywd->Fonts[FONTID_TRACY],str,postfix,100,YPACOLF_ALIGNRIGHT);
        };
    };

    /*** that's it ***/
    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_RenderLifebar(struct ypaworld_data *ywd,
                        struct YPAHud *h,
                        UBYTE *str,
                        struct Bacterium *b,
                        struct VehicleProto *vp,
                        FLOAT tx, FLOAT ty)
/*
**  FUNCTION
**      Rendert Lifebar zentriert an die angegebene Position.
**      Der Bakterien-Pointer darf NULL sein, in diesem Fall
**      wird die MaxEnergy aus dem VehicleProto genommen
**      (fuer Techupgrade Visualisierung).
**
**  CHANGED
**      15-Dec-97   floh    created
**      03-Apr-98   floh    + Energy-Wert wird jetzt korrekt gerundet
*/
{
    LONG energy, maximum;
    UBYTE *label;
    UBYTE buf[32];
    if (b) {
        energy  = b->Energy;
        maximum = b->Maximum;
    } else {
        energy = maximum = vp->Energy;
    };
    sprintf(buf,"%d",(energy+99)/100);
    label = ypa_GetStr(ywd->LocHandle,STR_HUD_HITPOINTS,"HP");
    str = yw_RenderTileBar(ywd,h,str,tx,ty,energy,maximum,2,6,label,buf);
    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_RenderShieldbar(struct ypaworld_data *ywd,
                          struct YPAHud *h,
                          UBYTE *str,
                          struct Bacterium *b,
                          struct VehicleProto *vp,
                          FLOAT tx, FLOAT ty) 
/*
**  CHANGED
**      15-Dec-97   floh    created
*/
{
    LONG shield;
    UBYTE *label;
    UBYTE buf[32];
    if (b) shield = b->Shield;
    else   shield = vp->Shield;
    sprintf(buf,"%d%%",shield);
    label = ypa_GetStr(ywd->LocHandle,STR_HUD_ARMOR,"AMR");
    str = yw_RenderTileBar(ywd,h,str,tx,ty,shield,100,1,5,label,buf);
    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_RenderReloadbar(struct ypaworld_data *ywd,
                          struct YPAHud *h,
                          UBYTE *str,
                          struct Bacterium   *b,
                          struct WeaponProto *wp,
                          FLOAT tx, FLOAT ty) 
/*
**  FUNCTION
**      Rendert Reload-Balken.
**
**  CHANGED
**      15-Dec-97   floh    created
**      20-Mar-98   floh    + Unterstützung für Salve-Shots.
*/
{
    if (wp) {
        LONG reload;
        UBYTE buf[64];
        UBYTE *label;
        ULONG wtime;
        if (b) {
            wtime = wp->ShotTime_User;
            if (wp->SalveShots) {
                if (b->salve_count >= wp->SalveShots) wtime = wp->SalveDelay;
            };
            reload = ((b->internal_time-b->last_weapon)*100) / wtime;
            if (reload >= 100) reload=100;
        } else reload = 100;
        if (reload == 100) sprintf(buf,ypa_GetStr(ywd->LocHandle,STR_HUD_RLDOK,"OK"));
        else               sprintf(buf,"%d%%",reload);
        label = ypa_GetStr(ywd->LocHandle,STR_HUD_RELOAD,"RLD");
        str = yw_RenderTileBar(ywd,h,str,tx,ty,reload,100,1,3,label,buf);
    };
    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_RenderWeaponbar(struct ypaworld_data *ywd,
                          struct YPAHud *h,
                          UBYTE *str,
                          struct Bacterium *b,
                          struct VehicleProto *vp,
                          struct WeaponProto *wp,
                          FLOAT tx, FLOAT ty)
/*
**  CHANGED
**      15-Dec-97   floh    created
*/
{
    if (wp) {
        LONG damage = wp->Energy;
        UBYTE buf[64];
        UBYTE *label;
        if (vp->NumWeapons>1) sprintf(buf,"%d x%d",damage/100,vp->NumWeapons);
        else                  sprintf(buf,"%d",damage/100);
        label = ypa_GetStr(ywd->LocHandle,STR_HUD_WPNDAMAGE,"DMG");
        str = yw_RenderTileBar(ywd,h,str,tx,ty,damage,100,7,7,label,buf);
    };
    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_RenderTextbar(struct ypaworld_data *ywd,
                        struct YPAHud *h,
                        UBYTE *str,
                        UBYTE *name,
                        FLOAT tx, FLOAT ty)
/*
**  FUNCTION
**      Rendert Text mit Rahmen drumrum, für Vehicle-Info-
**      und Build-Info-Display
**
**  CHANGED
**      10-Feb-98   floh    created
**      30-Apr-98   floh    Text sollte jetzt gegen Map clippen
*/
{
    WORD xpos = tx * (ywd->DspXRes>>1);
    WORD ypos = ty * (ywd->DspYRes>>1);
    WORD mx,my;
    FLOAT sx,sy,m11,m12,m21,m22;
    WORD w = h->vid_w * ywd->DspXRes;
    struct Skeleton *sklt = NULL;
    ULONG color1 = yw_GetColor(ywd,YPACOLOR_HUD_VEHICLE_0);
    ULONG color0 = yw_GetColor(ywd,YPACOLOR_HUD_VEHICLE_1);
    void (*render_hook)(struct ypaworld_data *,FLOAT,FLOAT,
        FLOAT,FLOAT,ULONG *,ULONG *) = h->is_hicolor ? yw_L2RScanColorHook:NULL;
    LONG clip_x0,clip_y0,clip_x1,clip_y1;
    BOOL do_map_clip;

    /*** falls Map offen, gegen Map clippen ***/
    clip_x0 = clip_x1 = 0;
    clip_y0 = clip_y1 = 0;
    do_map_clip = FALSE;
    if (!(MR.req.flags & REQF_Closed)) {
        do_map_clip = TRUE;
        clip_x0 = MR.req.req_cbox.rect.x - (ywd->DspXRes>>1);
        clip_x1 = clip_x0 + MR.req.req_cbox.rect.w;
        clip_y0 = MR.req.req_cbox.rect.y - (ywd->DspYRes>>1);
        clip_y1 = clip_y0 + MR.req.req_cbox.rect.h;
    }; 

    /*** Namen rendern ***/
    mx = xpos;
    my = ypos;
    xpos -= (w>>1);
    ypos -= (ywd->FontH>>1);
    if (!(do_map_clip && (mx>clip_x0) && (mx<clip_x1) && (my>clip_y0) && (my<clip_y1))) {
        new_font(str,FONTID_TRACY);
        pos_abs(str,xpos,ypos);
        dbcs_color(str,yw_Red(ywd,YPACOLOR_TEXT_HUD),yw_Green(ywd,YPACOLOR_TEXT_HUD),yw_Blue(ywd,YPACOLOR_TEXT_HUD));
        str = yw_TextBuildCenteredItem(ywd->Fonts[FONTID_TRACY],str,name,w,' ');
    };
    
    /*** Rahmen drum, fertig ***/
    sx = h->vid_w;
    sy = h->vid_text_h * 1.2;
    sklt = h->HudVecSklt[HUDVEC_FRAME];
    if (sklt) {
        FLOAT offset = (1.0 / ywd->DspYRes) * 2;
        m11 = 1.0;  m12 = 0.0;
        m21 = 0.0;  m22 = 1.0;
        yw_SetInterpolateColors(ywd,color0,color1);
        yw_VectorOutline(ywd,sklt,tx,ty+offset,m11,m12,m21,m22,sx,sy,color0,NULL,render_hook);
    };

    /*** fertig ***/
    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_RenderNamebar(struct ypaworld_data *ywd,
                        struct YPAHud *h,
                        UBYTE *str,
                        struct Bacterium *b,
                        struct VehicleProto *vp,
                        ULONG vpnum,
                        FLOAT tx, FLOAT ty)
/*
**  CHANGED
**      15-Dec-97   floh    created
*/
{
    UBYTE *name = ypa_GetStr(ywd->LocHandle,STR_NAME_VEHICLES+vpnum,vp->Name);
    str = yw_RenderTextbar(ywd,h,str,name,tx,ty);
    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_RenderVID(struct ypaworld_data *ywd,
                    struct YPAHud *h,
                    UBYTE *str,
                    FLOAT tx, FLOAT ty,
                    struct Bacterium *b,
                    LONG vp_num,
                    ULONG flags)
/*
**  FUNCTION
**      Rendert ein VehicleInfo Display an der angegebenen
**      Position. Es muss entweder ein Bakterien-Pointer
**      oder ein VehicleProto-Pointer angegeben werden.
**
**  CHANGED
**      14-Dec-97   floh    created
**      
*/
{
    FLOAT dt;
    struct VehicleProto *vp;
    struct WeaponProto *wp;
    FLOAT sx,sy,m11,m12,m21,m22;
    ULONG color1 = yw_GetColor(ywd,YPACOLOR_HUD_VEHICLE_0);
    ULONG color0 = yw_GetColor(ywd,YPACOLOR_HUD_VEHICLE_1);
    struct Skeleton *sklt = NULL;
    void (*render_hook)(struct ypaworld_data *,FLOAT,FLOAT,
        FLOAT,FLOAT,ULONG *,ULONG *) = h->is_hicolor ? yw_L2RScanColorHook:NULL;
    BOOL do_weapon  = TRUE;
    BOOL do_shield  = TRUE;
    BOOL do_energy  = TRUE;
    BOOL do_vehicle = TRUE;
    BOOL blink;

    /*** Techupgrade-Blink-Effekte ***/
    if ((ywd->TimeStamp/200) & 1) blink=TRUE;
    else                          blink=FALSE;
    if (flags & HUDVID_WEAPONBLINK) do_weapon=blink;
    if (flags & HUDVID_ENERGYBLINK) do_energy=blink;
    if (flags & HUDVID_SHIELDBLINK) do_shield=blink;
    if (flags & HUDVID_NAMEBLINK)   do_vehicle=blink;

    /*** Scaleup Timer ***/
    if (flags & HUDVID_SCALEUP) {
        dt = (((LONG)(ywd->TimeStamp-h->change_vhcl_timer))-200)/180.0;
        if (dt <= 0.0) return(str);
    } else {
        dt = 1.0;
    };

    /*** benoetigte Infos holen ***/
    if (vp_num == -1) vp_num=b->TypeID; 
    vp = &(ywd->VP_Array[vp_num]);
    if (vp->Weapons[0] != -1) wp = &(ywd->WP_Array[vp->Weapons[0]]);
    else                      wp = NULL;

    /*** und die einzelnen Elemente rendern ***/
    if (do_vehicle) yw_VecRenderVehicle(ywd,h,vp,tx,ty,dt);
    if (do_energy)  str = yw_RenderLifebar(ywd,h,str,b,vp,tx,ty+(7*h->vid_text_h));
    if (do_shield)  str = yw_RenderShieldbar(ywd,h,str,b,vp,tx,ty+(9*h->vid_text_h));
    if (do_vehicle) str = yw_RenderNamebar(ywd,h,str,b,vp,vp_num,tx,ty+(12*h->vid_text_h));
    if (wp && do_weapon) {
        yw_VecRenderWeapon(ywd,h,wp,tx,ty-(9*h->vid_text_h));
        str = yw_RenderReloadbar(ywd,h,str,b,wp,tx,ty-(7*h->vid_text_h));
        str = yw_RenderWeaponbar(ywd,h,str,b,vp,wp,tx,ty-(9*h->vid_text_h));
    };        
    
    /*** Ende ***/
    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_RenderBID(struct ypaworld_data *ywd,
                    struct YPAHud *h,
                    UBYTE *str,
                    FLOAT tx, FLOAT ty,
                    LONG bp_num,
                    ULONG flags)
/*
**  FUNCTION
**      Rendert ein BuildInfo Display an der angegebenen
**      Position. Es muss entweder ein Bakterien-Pointer
**      oder ein VehicleProto-Pointer angegeben werden.
**
**  CHANGED
**      10-Feb-98   floh    created
**      26-May-98   floh    oops, Gebaeude-Name war noch nicht lokalisiert...
*/
{
    FLOAT dt;
    struct BuildProto *bp;
    FLOAT sx,sy,m11,m12,m21,m22;
    ULONG color1 = yw_GetColor(ywd,YPACOLOR_HUD_VEHICLE_0);
    ULONG color0 = yw_GetColor(ywd,YPACOLOR_HUD_VEHICLE_1);
    void (*render_hook)(struct ypaworld_data *,FLOAT,FLOAT,
        FLOAT,FLOAT,ULONG *,ULONG *) = h->is_hicolor ? yw_L2RScanColorHook:NULL;
    BOOL blink;

    /*** Techupgrade-Blink-Effekte ***/
    if ((ywd->TimeStamp/200) & 1) blink=TRUE;
    else                          blink=FALSE;

    /*** Scaleup Timer ***/
    if (flags & HUDVID_SCALEUP) {
        dt = (((LONG)(ywd->TimeStamp-h->change_vhcl_timer))-200)/180.0;
        if (dt <= 0.0) return(str);
    } else {
        dt = 1.0;
    };

    /*** benoetigte Infos holen ***/
    bp = &(ywd->BP_Array[bp_num]);

    /*** und die einzelnen Elemente rendern ***/
    if (blink) {
        UBYTE *name;
        if (ywd->playing_network) name = ypa_GetStr(ywd->LocHandle,STR_NAME_NETWORK_BUILDINGS+bp_num,bp->Name);
        else                      name = ypa_GetStr(ywd->LocHandle,STR_NAME_BUILDINGS+bp_num,bp->Name);
        str = yw_RenderTextbar(ywd,h,str,name,tx,ty);
    };

    /*** Ende ***/
    return(str);
}

/*-----------------------------------------------------------------*/
void yw_VecRenderVisor(struct ypaworld_data *ywd, struct YPAHud *h)
/*
**  CHANGED
**      29-Apr-97   floh    created
**      31-Jul-97   floh    pulsiert jetzt
**      06-Aug-97   floh    + Bugfix: Fahrzeuge ohne MG sollten
**                            das autonome Visier jetzt korrekt anzeigen
**      09-Dec-97   floh    + yw_GetColor()
*/
{
    FLOAT tx,ty,sx,sy,m11,m12,m21,m22;
    ULONG color0,color1;
    struct Skeleton *sklt = NULL;
    void (*render_hook)(struct ypaworld_data *,FLOAT,FLOAT,FLOAT,FLOAT,ULONG *,ULONG *);
    FLOAT dtimer = (((LONG)(ywd->TimeStamp-h->change_vhcl_timer))-350)/200.0;
    if (dtimer <= 0.0) return;

    /*** Timestamp zurücksetzen ***/
    if (ywd->visor.target) {
        if (h->prev_target != ywd->visor.target) {
            h->prev_target = ywd->visor.target;
            h->change_target_timer = ywd->TimeStamp;
        };
    } else {
        h->prev_target = NULL;
    };

    /*** MG-Visier rendern ***/
    if (ywd->visor.mg_type) {
        color1 = yw_GetColor(ywd,YPACOLOR_HUD_VISOR_MG_0);
        color0 = yw_GetColor(ywd,YPACOLOR_HUD_VISOR_MG_1);
        tx  = ywd->visor.x;
        ty  = ywd->visor.y;
        sx  = 0.3;
        sy  = 0.4;
        if (dtimer < 1.4) {
            sx*=dtimer;
        };
        m11 = 1.0;  m12 = 0.0;
        m21 = 0.0;  m22 = 1.0;
        sklt = h->HudVecSklt[HUDVEC_MG_VISOR];
        if (sklt) {
            render_hook = h->is_hicolor ? yw_SlowPulsateColorHook : NULL;
            yw_SetInterpolateColors(ywd,color0,color1);
            yw_VectorOutline(ywd,sklt,
                             tx, ty,
                             m11, m12, m21, m22,
                             sx, sy, color0, render_hook, NULL);
        };
    };

    /*** Visier fuer autonome Waffe anzeigen ***/
    if (ywd->visor.gun_type) {

        #define NUM_DT (2)
        FLOAT dt[NUM_DT];
        ULONG num_dt = 0;
        ULONG i;

        /*** ungelockt, oder Lock-Zoom? ***/
        if (ywd->visor.target) {
            num_dt = NUM_DT;
            for (i=0; i<num_dt; i++) {
                FLOAT timer = 200.0;
                dt[i] = ((LONG)(ywd->TimeStamp - h->change_target_timer))/timer;
                if (dt[i] > 1.0) dt[i]=1.0;
            };
            color0 = yw_GetColor(ywd,YPACOLOR_HUD_VISOR_LOCKED_0);
            color1 = yw_GetColor(ywd,YPACOLOR_HUD_VISOR_LOCKED_1);
            render_hook = h->is_hicolor ? yw_FastPulsateColorHook : NULL;
        } else {
            dt[0]  = 0.0;
            dt[1]  = 0.0;
            num_dt = 2;
            color1 = yw_GetColor(ywd,YPACOLOR_HUD_VISOR_AUTONOM_0);
            color0 = yw_GetColor(ywd,YPACOLOR_HUD_VISOR_AUTONOM_1);
            render_hook = h->is_hicolor ? yw_SlowPulsateColorHook : NULL;
        };
        for (i=0; i<num_dt; i++) {
            FLOAT rad;
            if (ywd->visor.target) {
                tx = ywd->visor.x + ((ywd->visor.gun_x-ywd->visor.x)*dt[i]);
                ty = ywd->visor.y + ((ywd->visor.gun_y-ywd->visor.y)*dt[i]);
            } else {
                tx = ywd->visor.gun_x;
                ty = ywd->visor.gun_y;
            };
            sx = 0.3 + ((0.13 - 0.3)*dt[i]);
            sy = 0.4 + ((0.17 - 0.4)*dt[i]);
            if (dtimer < 1.4) {
                sy *= dtimer;
            };
            if (i & 1) rad = 0.0 + (( 1.571 - 0.0)*dt[i]);
            else {
                if (dt[i] < 1.0) {
                    /*** Zoom noch ongoing ***/
                    rad = 0.0 + ((-3.141 - 0.0)*dt[i]);
                } else {
                    /*** bereits aufgelockt, ungerades Elm weiterdrehen ***/
                    rad = (ywd->TimeStamp - h->change_target_timer) / 1000.0;
                    rad *= 6.282;
                };
            };
            m11 = cos(rad); m12 = -sin(rad);
            m21 = sin(rad); m22 = cos(rad);

            switch(ywd->visor.gun_type) {
                case VISORTYPE_GRENADE:
                    if (i&1) sklt = h->HudVecSklt[HUDVEC_GRENADE_VISOR_1];
                    else     sklt = h->HudVecSklt[HUDVEC_GRENADE_VISOR_2];
                    break;
                case VISORTYPE_ROCKET:
                    if (i&1) sklt = h->HudVecSklt[HUDVEC_ROCKET_VISOR_1];
                    else     sklt = h->HudVecSklt[HUDVEC_ROCKET_VISOR_2];
                    break;
                case VISORTYPE_MISSILE:
                    if (i&1) sklt = h->HudVecSklt[HUDVEC_MISSILE_VISOR_1];
                    else     sklt = h->HudVecSklt[HUDVEC_MISSILE_VISOR_2];
                    break;
                default:
                    sklt = NULL;
                    break;
            };

            if (sklt) {
                yw_SetInterpolateColors(ywd,color0,color1);
                yw_VectorOutline(ywd,sklt,
                                 tx, ty,
                                 m11, m12, m21, m22,
                                 sx, sy, color0, render_hook, NULL);
            };
        };
    };
}

/*-----------------------------------------------------------------*/
UBYTE *yw_3DOverlayCursors(struct ypaworld_data *ywd, UBYTE *str)
/*
**  CHANGED
**      17-Dec-97   floh    created 
**      21-May-98   floh    + Anzeige des Multiplayer-Namens   
**      02-Jun-98   floh    + beachtet jetzt das YPA_PREFS_INDICATOR
**                            Flag  
*/
{
    if (ywd->Prefs.Flags & YPA_PREFS_INDICATOR) {

        LONG sec_x,sec_y,end_x,end_y,store_sec_x;

        /*** Start-Sektor und End-Sektor ermitteln ***/
        sec_x = ywd->UVBact->SectX-1;
        sec_y = ywd->UVBact->SectY-1;
        end_x = ywd->UVBact->SectX+1;
        end_y = ywd->UVBact->SectY+1;
        if (sec_x < 1) sec_x = 1;
        if (sec_y < 1) sec_y = 1;
        if (end_x >= ywd->MapSizeX) end_x = ywd->MapSizeX-1;
        if (end_y >= ywd->MapSizeY) end_y = ywd->MapSizeY-1;

        /*** oki doki, also mal die Sektoren scannen... ***/
        store_sec_x = sec_x;
        for (sec_y; sec_y <= end_y; sec_y++) {
            for (sec_x = store_sec_x; sec_x <= end_x; sec_x++) {

                struct Cell *sec = &(ywd->CellArea[sec_y * ywd->MapSizeX + sec_x]);
                struct MinList *ls;
                struct MinNode *nd;
        
                /*** fuer jede Bakterie im Sektor... ***/
                if (sec->FootPrint & MR.footprint) {
                    ls = (struct MinList *) &(sec->BactList);
                    for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {
                        struct Bacterium *b = (struct Bacterium *) nd;
                        if ((b->BactClassID != BCLID_YPAMISSY) &&
                            (b->BactClassID != BCLID_YPAROBO)  &&
                            (b->MainState   != ACTION_CREATE)  &&
                            (b->MainState   != ACTION_BEAM)    &&
                            (b->MainState   != ACTION_DEAD))
                        {
                            if (BCLID_YPAGUN == b->BactClassID) {
                                struct ypagun_data *yd;
                                yd = INST_DATA( ((struct nucleusdata *)b->BactObject)->o_Class, b->BactObject);
                                if (!(yd->flags & GUN_RoboGun)) yw_3DCursorOverBact(ywd,b);
                            } else yw_3DCursorOverBact(ywd,b);
                            
                            /*** handelt es sich hier um einen Multiplayer-Opponenten? ***/
                            if (ywd->playing_network && (b->ExtraState & EXTRA_ISVIEWER) && (ywd->gsr)) {
                                str = yw_3DNameOverBact(ywd,str,b);
                            };    
                        };
                    };
                };
            };
        };
    };
    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_RenderHUDVecs(struct ypaworld_data *ywd, UBYTE *str)
/*
**  FUNCTION
**      Rendert alle HUD-Vektoren.
**
**  CHANGED
**      22-Apr-97   floh    created
**      14-Dec-97   floh    + yw_RenderVID()
**      20-May-98   floh    + Visor wird nicht mehr gegen Map geclippt
*/
{
    struct YPAHud *h = &(ywd->Hud);
    struct rast_rect rr;
    struct rast_intrect inv_rr;
    struct rast_intrect zero_inv_rr;

    /*** Cliprect voll aufziehen ***/
    rr.xmin = -1.0;  rr.xmax = +1.0;
    rr.ymin = -1.0;  rr.ymax = +1.0;
    _methoda(ywd->GfxObject, RASTM_ClipRegion, &rr);
    
    /*** manche HUD-Elemente werden nicht gegen die Map geclippt... ***/    
    zero_inv_rr.xmin = 0;
    zero_inv_rr.xmax = 0;
    zero_inv_rr.ymin = 0;
    zero_inv_rr.ymax = 0;    

    /*** inverses Cliprect auf Map  (falls offen) ***/
    if (!(MR.req.flags & REQF_Closed)) {
        inv_rr.xmin = MR.req.req_cbox.rect.x - (ywd->DspXRes>>1); 
        inv_rr.ymin = MR.req.req_cbox.rect.y - (ywd->DspYRes>>1);
        inv_rr.xmax = inv_rr.xmin + MR.req.req_cbox.rect.w;
        inv_rr.ymax = inv_rr.ymin + MR.req.req_cbox.rect.h;
    } else {
        inv_rr.xmin = 0;
        inv_rr.xmax = 0;
        inv_rr.ymin = 0;
        inv_rr.ymax = 0;
    };

    /*** HUD rendern ***/
    _methoda(ywd->GfxObject, RASTM_IntInvClipRegion, &inv_rr);
    yw_VecRenderCompass(ywd,h);
    _methoda(ywd->GfxObject, RASTM_IntInvClipRegion, &zero_inv_rr);
    yw_VecRenderVisor(ywd,h);
    _methoda(ywd->GfxObject, RASTM_IntInvClipRegion, &inv_rr);
    str = yw_RenderVID(ywd,h,str,-0.7,0.3,ywd->UVBact,-1,HUDVID_SCALEUP);

    /*** ClipRect zuruecksetzten ***/
    if (!(MR.req.flags & REQF_Closed)) {
        inv_rr.xmin = 0;
        inv_rr.xmax = 0;
        inv_rr.ymin = 0;
        inv_rr.ymax = 0;
        _methoda(ywd->GfxObject, RASTM_IntInvClipRegion, &inv_rr);
    };
    return(str);
}

/*-----------------------------------------------------------------*/
void yw_RenderHUD(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Falls User in einem normalen Vehikel sitzt,
**      wird das HUD gerendert.
**
**  CHANGED
**      01-Sep-96   floh    created
**      02-Sep-96   floh    die "üblichen" HUD-Elemente werden
**                          nur noch gezeichnet, wenn das
**                          User-Vehikel keine Flak ist.
**      17-Sep-96   floh    in Guns werden jetzt auch die
**                          HUD-Energie-Balken angezeigt,
**                          weil das Radar so einseitig aussah.
**      03-Nov-96   floh    wenn tot, kein HUD, aber blinkende
**                          Meldung
**      08-Apr-97   floh    künstlicher Horizont ist raus
**      30-Apr-97   floh    radikal aufgeräumt
**      01-Aug-97   floh    + "Atemfrequenz" für HUD.
**      24-Nov-97   floh    + blinkende Todesmeldung DBCS enabled
**      10-Dec-97   floh    + Tot-Message eingefaerbt
**      17-Dec-97   floh    + yw_3DOverlayCursors()
**      02-Jun-98   floh    + Drone Destroyed Meldung jetzt in weiss
*/
{
    UBYTE *str = YPA_HudStr;
    struct drawtext_args dt;

    /*** tot? ***/
    if (ACTION_DEAD == ywd->UVBact->MainState) {

        UBYTE *dmsg = NULL;

        if (ywd->URBact->ExtraState & EXTRA_CLEANUP) {
            /*** im Beamzustand ***/
            dmsg = NULL;
        } else if (ywd->URBact->MainState == ACTION_DEAD) {
            /*** Robo-Tot-Message auch in "anderen" Vehikeln ***/
            dmsg = ypa_GetStr(ywd->LocHandle,STR_ROBOBROKEN,
            "* * *  H O S T  S T A T I O N  D E S T R O Y E D  * * *");
        } else {
            dmsg = ypa_GetStr(ywd->LocHandle,STR_DRONEBROKEN,
            "* * *  D R O N E  D E S T R O Y E D  * * *");
        };

        if (dmsg) {
            /*** Message blinkend drucken ***/
            if ((ywd->TimeStamp / 500) & 1) {
                WORD ypos = -(ywd->FontH>>1);   // genau in vertikaler Mitte
                new_font(str,FONTID_TRACY);
                xpos_brel(str,0);
                ypos_abs(str,ypos);
                dbcs_color(str,255,255,255);
                str = yw_TextCenteredSkippedItem(ywd->Fonts[FONTID_TRACY],
                                                 str, dmsg, ywd->DspXRes);
            };
        };

    } else if ((ywd->UserRobo != ywd->UserVehicle) && (ywd->Hud.RenderHUD)) {

        struct YPAHud *h = &(ywd->Hud);
        FLOAT breath_d;

        /*** Radar layouten ***/
        yw_RenderRadar(ywd);

        /*** Atemfrequenz berechnen ***/
        if (ywd->UVBact->Energy < ywd->UVBact->Maximum) {
            h->breath_time = (3000 * ywd->UVBact->Energy) / ywd->UVBact->Maximum;
            if (h->breath_time < 200) h->breath_time=200;
        } else {
            h->breath_time = 3000;
        };
        h->breath_pos += ((FLOAT)(ywd->FrameTime)) / ((FLOAT)(h->breath_time));
        if (h->breath_pos >= 1.0) h->breath_pos=0.0; 

        /*** 2D-Vektor-Outlines rendern ***/
        str = yw_RenderHUDVecs(ywd,str);
    };

    /*** Potential Selected Cursor ***/
    if (ywd->FrameFlags & YWFF_MouseOverBact) {
        yw_3DCursorOverBact(ywd,ywd->SelBact);
        str = yw_3DLifebarOverBact(ywd,str,ywd->SelBact);
    };

    /*** Lifebar ueber Targetted Vehicle ***/
    if (ywd->visor.target) {
        str = yw_3DLifebarOverBact(ywd,str,ywd->visor.target);
    };
    
    /*** Overlay-Cursors ueber Vehikel ***/
    str = yw_3DOverlayCursors(ywd,str);

    /*** EOS ***/
    eos(str);

    /*** alles rendern ***/
    dt.string = YPA_HudStr;
    dt.clips  = NULL;
    _DrawText(&dt);

    /*** loesche Visier-Target, weil dieses manchmal "haengen bleibt" ***/
    ywd->visor.target = NULL; 
}

/*-----------------------------------------------------------------*/
BOOL yw_RenderTechUpgrade(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Falls in den letzten N Sekunden ein Techupgrade
**      passiert, wird ein VehicleInformation-Display
**      angezeigt.
**
**  CHANGED
**      16-Dec-97   floh    created
**      15-Jan-98   floh    Bugfix: Cliprect wurde nicht aufgezogen.
**      10-Feb-98   floh    + Support für Buildprotos.
**                          + Waffen-Upgrades wurden nicht korrekt
**                            angezeigt.
*/
{
    UBYTE *str = YPA_HudStr;
    struct YPAHud *h = &(ywd->Hud);
    struct rast_text rt;
    BOOL retval = FALSE;
    struct rast_rect rr;

    /*** Cliprect voll aufziehen ***/
    rr.xmin = -1.0;  rr.xmax = +1.0;
    rr.ymin = -1.0;  rr.ymax = +1.0;
    _methoda(ywd->GfxObject, RASTM_ClipRegion, &rr);

    if ((ywd->touch_stone.gem != -1) && 
        ((ywd->TimeStamp - ywd->touch_stone.time_stamp) < 10000) &&
        (ywd->touch_stone.vp_num || ywd->touch_stone.wp_num || ywd->touch_stone.bp_num))
    {
        ULONG flags = 0;
        ULONG vp_num = ywd->touch_stone.vp_num;
        ULONG wp_num = ywd->touch_stone.wp_num;
        ULONG bp_num = ywd->touch_stone.bp_num;
        if (!vp_num) {
            /*** das war ein Waffen-Upgrade, zugehöriges Vehikel suchen ***/
            if (wp_num) {
                ULONG i;
                for (i=0; ((i<NUM_VEHICLEPROTOS)&&(!vp_num)); i++) {
                    /*** es iss ein normales Vehicle ***/
                    if (ywd->VP_Array[i].Weapons[0] == wp_num) {
                        vp_num = i;
                    };
                };
                if (i == NUM_VEHICLEPROTOS) {
                    /*** diese Waffe verwendet niemand... ***/
                    return(FALSE);
                };
            };
        };
        h->breath_pos = 1.0;
        switch(ywd->gem[ywd->touch_stone.gem].type) {
            case LOGMSG_TECH_WEAPON:    flags |= HUDVID_WEAPONBLINK; break; 
            case LOGMSG_TECH_ARMOR:     flags |= HUDVID_SHIELDBLINK; break;
            case LOGMSG_TECH_VEHICLE:   flags |= HUDVID_NAMEBLINK;   break;
        };
        if (vp_num)      str = yw_RenderVID(ywd,h,str,0.0,-0.5,NULL,vp_num,flags);
        else if (bp_num) str = yw_RenderBID(ywd,h,str,0.0,-0.5,bp_num,flags);
        eos(str);
        rt.string = YPA_HudStr;
        rt.clips  = NULL;
        _methoda(ywd->GfxObject,RASTM_Text,&rt);
        retval = TRUE;
    };
    return(retval);
}

