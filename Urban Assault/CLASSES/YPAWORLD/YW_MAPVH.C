/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_mapvhcl.c,v $
**  $Revision: 38.5 $
**  $Date: 1998/01/06 16:24:04 $
**  $Locker: floh $
**  $Author: floh $
**
**  Abgesplittete Map-/Radar-Vehicle-Render-Routinen.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <stdio.h>
#include <math.h>

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "engine/engine.h"
#include "ypa/ypaworldclass.h"
#include "ypa/guimap.h"
#include "ypa/guilogwin.h"
#include "ypa/ypavehicles.h"

#include "ypa/ypagunclass.h"
#include "ypa/yparoboclass.h"       // für Debug-Anzeigen

#include "yw_protos.h"

#ifdef __WINDOWS__
extern unsigned long wdd_DoDirect3D
#endif

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_ov_engine

extern struct YPAStatusReq SR;
extern struct YPAMapReq MR;     // aus yw_mapreq.c
extern struct YPALogWin LW;
extern UBYTE RadarStr[];

#ifdef AMIGA
extern __far UBYTE MR_ClipRealMap[];  // Char-Buffer für Map-Interior
#else
extern UBYTE MR_ClipRealMap[];  // Char-Buffer für Map-Interior
#endif

/*-----------------------------------------------------------------*/
void yw_MapGetXZ(FLOAT x, FLOAT z, LONG *disp_x, LONG *disp_y)
/*
**  FUNCTION
**      Transformiert ein Welt-Koordinaten-Paar in
**      eine gültige Display-Koordinate. Folgende
**      Map-Parameter müssen gültig sein:
**
**      MR.ps_x,ps_y
**      MR.pw_x,pw_y
**      MR.topleft_x,topleft_y
**      MR.x_aspect
**      MR.y_aspect
**      MR.r_dir
**      MR.midx,midz
**
**      Im Radar-Modus muß MAPF_RADAR_MODE gesetzt sein!
**
**  CHANGED
**      15-Sep-96   floh    created
*/
{
    if (MR.flags & MAPF_RADAR_MODE) {
        /*** im Radar-Modus zuerst um midx rotieren ***/
        FLOAT tx,tz;
        x -= MR.midx;
        z -= MR.midz;
        tx = MR.r_dir.m11*x + MR.r_dir.m13*z;
        tz = MR.r_dir.m31*x + MR.r_dir.m33*z;
        x = tx + MR.midx;
        z = tz + MR.midz;
    };
    *disp_x = (FLOAT_TO_INT(x  / MR.x_aspect) - MR.ps_x) + MR.topleft_x;
    *disp_y = (FLOAT_TO_INT(-z / MR.y_aspect) - MR.ps_y) + MR.topleft_y;
}

/*-----------------------------------------------------------------*/
void yw_MapLine(Object *gfxo,
                FLOAT x0, FLOAT z0,
                FLOAT x1, FLOAT z1,
                ULONG color)
/*
**  FUNCTION
**      Zeichnet eine geclippte Linie in der Map zwischen
**      den 3D-Koordinaten [x0,z0],[x1,z1]
**
**  CHANGED
**      20-May-96   floh    created
**      30-Jul-96   floh    revised & updated (Dynamic Layout + Locale)
**      15-Sep-96   floh    + Koordinaten-Transformation mit Hilfe
**                            von yw_MapGetXZ()
**      09-Jun-97   floh    + Owner-RGB-VecPen-Farben korrigiert
**      01-Jun-97   floh    + Fix für Start- und Endpunkt-Farbe
**      09-Dec-97   floh    + 8-Bit-Colorhack rausgenommen
*/
{
    struct rast_intline rl;
    struct rast_pens rp;

    /*** Display-Koordinaten berechnen ***/
    yw_MapGetXZ(x0,z0,&(rl.x0),&(rl.y0));
    yw_MapGetXZ(x1,z1,&(rl.x1),&(rl.y1));
    rp.fg_pen  = color;
    rp.fg_apen = color;
    rp.bg_pen  = -1;
    _methoda(gfxo,RASTM_SetPens,&rp);
    _methoda(gfxo,RASTM_IntClippedLine,&rl);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_MapChar(UBYTE *str,
                  FLOAT x0, FLOAT z0,
                  UBYTE chr,
                  UWORD width,
                  UWORD height)
/*
**  FUNCTION
**      Layoutet ein Char mit Positions-Angabe im
**      Welt-Koordinaten-System. Der Font muß bereits
**      initialisiert sein!
**
**  CHANGED
**      15-Sep-96   floh    created
*/
{
    LONG mx,my,sx,sy,ex,ey;
    LONG skip_x,skip_y,len_x,len_y;

    /*** Positions-Offset zu Top-Left der Karte ***/
    yw_MapGetXZ(x0,z0,&mx,&my);
    sx = mx - (width>>1) - MR.topleft_x;
    sy = my - (height>>1) - MR.topleft_y;
    ex = sx + width;
    ey = sy + height;

    /*** nicht sichtbar? ***/
    if ((sx>=MR.pw_x) || (sy>=MR.pw_y) || (ex<=0) || (ey<=0)) return(str);

    /*** Clip-Parameter ***/
    if (sx < 0) { skip_x=-sx; sx=0; }
    else          skip_x=0;
    if (sy < 0) { skip_y=-sy; sy=0; }
    else          skip_y=0;
    if (ex > MR.pw_x) len_x = MR.pw_x - sx;
    else              len_x = 0;
    if (ey > MR.pw_y) len_y = MR.pw_y - sy;
    else              len_y = 0;

    /*** String konstruieren ***/
    sx += MR.topleft_x;
    sy += MR.topleft_y;
    pos_abs(str,sx,sy);

    /*** Clipping ***/
    if (skip_y != 0) {
        off_vert(str,skip_y);
    } else if (len_y != 0) {
        len_vert(str,len_y);
    };
    if (skip_x != 0) {
        off_hori(str,skip_x);
    } else if (len_x != 0) {
        len_hori(str,len_x);
    };

    /*** und finally, das Char itself ***/
    put(str,chr);

    /*** Ende ***/
    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_MapLenVertChar(UBYTE *str,
                         FLOAT x0, FLOAT z0,
                         UBYTE chr, UWORD size,
                         UWORD len_vert)
/*
**  FUNCTION
**      Entspricht yw_MapChar() rendert aber nur <len_vert>
**      Pixel des oberen Teils.
**
**  CHANGED
**      03-Jul-97   floh    created
*/
{
    LONG mx,my,sx,sy,ex,ey;
    LONG skip_x,skip_y,len_x,len_y;

    /*** Positions-Offset zu Top-Left der Karte ***/
    yw_MapGetXZ(x0,z0,&mx,&my);
    sx = mx - (size>>1) - MR.topleft_x;
    sy = my - (size>>1) - MR.topleft_y;
    ex = sx + size;
    ey = sy + len_vert;

    /*** nicht sichtbar? ***/
    if ((sx>=MR.pw_x) || (sy>=MR.pw_y) || (ex<=0) || (ey<=0)) return(str);

    /*** Clip-Parameter ***/
    if (sx < 0) { skip_x=-sx; sx=0; }
    else          skip_x=0;
    if (sy < 0) { skip_y=-sy; sy=0; }
    else          skip_y=0;
    if (ex > MR.pw_x) len_x = MR.pw_x - sx;
    else              len_x = 0;
    if (ey > MR.pw_y) len_y = MR.pw_y - sy;
    else              len_y = len_vert;

    /*** String konstruieren ***/
    sx += MR.topleft_x;
    sy += MR.topleft_y;
    pos_abs(str,sx,sy);

    /*** Clipping ***/
    if (skip_y != 0) {
        off_vert(str,skip_y);
    };
    if (len_y != 0) {       // kein Fehler, kann beides != 0 sein!
        len_vert(str,len_y);
    };
    if (skip_x != 0) {
        off_hori(str,skip_x);
    } else if (len_x != 0) {
        len_hori(str,len_x);
    };

    /*** und finally, das Char itself ***/
    put(str,chr);

    /*** Ende ***/
    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_MapFontChar(UBYTE *str, UBYTE fnt_id,
                      FLOAT x0, FLOAT z0,
                      UBYTE chr,
                      UWORD chr_width,
                      UWORD chr_height)
/*
**  FUNCTION
**      Exakt wie yw_MapChar() initialisiert aber den
**      Font selbst (dieser muß mit übergeben werden).
**
**  CHANGED
**      15-Sep-96   floh    created
**      08-Jul-97   floh    + Width / Height
*/
{
    new_font(str,fnt_id);
    str = yw_MapChar(str,x0,z0,chr,chr_width,chr_height);
    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_MapString(struct ypaworld_data *ywd,
                    UBYTE *text, UBYTE fnt_id, UBYTE *str, 
                    LONG sec_x, LONG sec_y)
/*
**  FUNCTION
**      Rendert einen String in die Map.
**
**  CHANGED
**      13-Jun-98   floh    created
*/
{
    LONG mx,my,sx,sy,ex,ey;
    LONG size_x,size_y;
    FLOAT x0,z0;

    /*** Ermittle Ausdehnung des Textes ***/
    size_x = yw_StrLen(text, ywd->Fonts[fnt_id]);
    size_y = ywd->Fonts[fnt_id]->height;

    /*** Positions-Offset zu Top-Left der Karte ***/
    x0 = (FLOAT) (sec_x * SECTOR_SIZE);
    z0 = (FLOAT) -(sec_y * SECTOR_SIZE);
    yw_MapGetXZ(x0,z0,&mx,&my);
    sx = mx - MR.topleft_x;
    sy = my - MR.topleft_y;
    ex = sx + size_x;
    ey = sy + size_y;

    /*** nicht sichtbar? ***/
    if ((sx <= 0) || (sy <= 0) || (ex>=MR.pw_x) || (ey>=MR.pw_y)) return(str);

    /*** String konstruieren ***/
    sx += MR.topleft_x;
    sy += MR.topleft_y;
    new_font(str,fnt_id);
    pos_abs(str,sx,sy);
    while (*str++ = *text++);
    str--;

    /*** Ende ***/
    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_RenderMapBact(struct ypaworld_data *ywd,
                        UBYTE *str,
                        UWORD char_width,
                        UWORD char_height,
                        struct Bacterium *b,
                        BOOL do_type_icon)
/*
**  FUNCTION
**      Spezialisierter Map-Vehikel-Renderer, handelt alles
**      ab...
**      Der richtige Font muß bereits eingestellt sein!
**
**  INPUTS
**      ywd
**      str
**      char_size
**      b
**      do_type_icon    - TRUE -> verwende Type-Icons (richtiger
**                        Font muß eingestellt sein, und
**                        char_size muß stimmen!
**
**  RESULTS
**      modifiziertes <str>
**
**  CHANGED
**      22-Nov-95   floh    created
**      05-Feb-96   floh    beschränkt auf 5x5 Bacts
**      06-Feb-96   floh    - auskommentierten Clipping-Code entfernt
**                          + Richtungs-Vektor wird gezeichnet
**                          - Bakterien werden nicht mehr immediate
**                            gezeichnet, sondern in Bakt-Buffer
**                          - Raketen werden nicht mehr gezeichnet
**      11-Feb-96   floh    Target-Vektor übermalt Richtungs-Vektor
**      13-Mar-96   floh    - feindliche Fahrzeuge -> kein Target-Vektor
**                            mehr
**                          + Richtungs-Vektor in "Wagenfarbe"
**      20-May-96   floh    + Target-Vektor wird jetzt erstmal als
**                            durchgezogene Linie gezeichnet, und
**                            damit logischerweise für alle Bakterien...
**                            Also raus aus dieser Routine...
**                          + Neue Fontgrößen, 3,5,7.
**                          + Selektetes Geschwader wird nicht mehr
**                            als "Font", sondern als Umrandung
**                            gezeichnet (nämlich in RenderMapCursors()).
**                          + Robos, Vehikel, Waffen haben jetzt
**                            jeweils unterschiedliche Chars.
**      30-Jul-96   floh    revised & updated (Dynamic Layout + Locale)
**      11-Sep-96   floh    + do_type_icon Arg
**      15-Sep-96   floh    + vereinfacht...
**      16-Sep-96   floh    + Richtungs-Vektor war nicht normalisiert
**      12-Apr-97   floh    + VP_Array nicht mehr global
**                          + WP_Array nicht mehr global
**      27-May-97   floh    + neue Typ-spezifische Icons.
**      09-Jun-97   floh    + Black Sect Vehikel (Owner 5) können auf
**                            der Map nicht mehr gesehen werden
**      27-Jun-97   floh    + TypeIcons reaktiviert
**      03-Jul-97   floh    + Robo-Vektor wieder kürzer
**                          + in Advanced Info Layer Lifemeters
**      08-Jul-97   floh    + bei Feindfahrzeugen wird keine Vehikel-
**                            Energie mehr angezeigt.
**      11-Jul-97   floh    + Waffen werden generell nicht mehr als TypeIcon
**                            gerendert.
**      18-Dec-97   floh    + neue Richtungs-Indikatoren
**      11-Jun-98   floh    + Feintuning an den Richtungspfeilen
*/
{
    UBYTE chr;

    /*** Er ist tot, Jim. ***/
    if (ACTION_DEAD == b->MainState) return(str);

    /*** Black Sect? ***/
    if (5 == b->Owner) return(str);

    /*** richtiges Char auswählen ***/
    if (do_type_icon) {
        switch(b->BactClassID) {
            case BCLID_YPAMISSY:
                chr = MAP_CHAR_WEAPON + b->Owner;
                break;
            default:
                chr = ywd->VP_Array[b->TypeID].TypeIcon;
                break;
        };
    } else {
        switch(b->BactClassID) {
            case BCLID_YPAROBO:
                {
                    switch(MR.zoom) {
                        case 0:
                        case 1:
                            chr = MAP_CHAR_ROBO_SMALL + b->Owner;
                            break;
                        case 2:
                            chr = MAP_CHAR_ROBO_MEDIUM + b->Owner;
                            break;
                        case 3:
                            chr = MAP_CHAR_ROBO_BIG + b->Owner;
                            break;
                        case 4:
                        default:
                            chr = MAP_CHAR_ROBO_HUGE + b->Owner;
                            break;
                    };
                };
                break;
            case BCLID_YPABACT:
                chr = MAP_CHAR_HELI  + b->Owner;
                break;
            case BCLID_YPATANK:
            case BCLID_YPACAR:
                chr = MAP_CHAR_TANK  + b->Owner;
                break;
            case BCLID_YPAFLYER:
                chr = MAP_CHAR_PLANE + b->Owner;
                break;
            case BCLID_YPAUFO:
                chr = MAP_CHAR_UFO   + b->Owner;
                break;
            case BCLID_YPAGUN:
                chr = MAP_CHAR_FLAK  + b->Owner;
                break;
            case BCLID_YPAMISSY:
                chr = MAP_CHAR_WEAPON + b->Owner;
                break;
            default:
                chr = 'A';
                break;
        };
    };

    /*** Positions-Offset zu Top-Left der Karte ***/
    str = yw_MapChar(str,b->pos.x,b->pos.z,chr,char_width,char_height);

    /*** Richtungs-Vektor (nicht bei Raketen) ***/
    if ((BCLID_YPAMISSY != b->BactClassID) && (MR.zoom > 2)) {

        FLOAT nx0,ny0,l0;
        FLOAT nx1,ny1,l1;
        FLOAT px0,py0;
        FLOAT px1,py1;
        FLOAT px2,py2;
        FLOAT mul0,mul1;
        ULONG color = yw_GetColor(ywd,YPACOLOR_OWNER_0+b->Owner);

        if (b == ywd->URBact) {
            /*** Sonderfall User-Robo: Vektor in Blickrichtung ***/
            nx0 = ywd->URBact->Viewer.dir.m11;
            ny0 = ywd->UVBact->Viewer.dir.m13;
            nx1 = ywd->URBact->Viewer.dir.m31;
            ny1 = ywd->UVBact->Viewer.dir.m33;
            mul0 = 85.0;
            mul1 = 700.0;
        } else if (b == ywd->UVBact) {
            /*** Sonderfall Viewer: etwas groesserer Vektor ***/
            nx0 = b->dir.m11;
            ny0 = b->dir.m13;
            nx1 = b->dir.m31;
            ny1 = b->dir.m33;
            mul0 = 60.0;
            mul1 = 500.0;
        } else {
            /*** alle restlichen Vehicle ***/
            nx0 = b->dir.m11;
            ny0 = b->dir.m13;
            nx1 = b->dir.m31;
            ny1 = b->dir.m33;
            mul0 = 40.0;
            mul1 = 350.0;
        };
        l0  = nc_sqrt(nx0*nx0 + ny0*ny0);
        if (l0 > 0.0) { 
            nx0 /= l0; ny0 /=l0; 
        } else {
            nx0 = 1.0; ny0 = 0.0;
        };
        l1  = nc_sqrt(nx1*nx1 + ny1*ny1);
        if (l1 > 0.0) { 
            nx1 /= l1; ny1 /=l1; 
        } else {
            nx1 = 0.0; ny1 = 1.0;
        };
        px0 =  nx0 * mul0 + b->pos.x;
        py0 =  ny0 * mul0 + b->pos.z;
        px1 =  nx1 * mul1 + b->pos.x;
        py1 =  ny1 * mul1 + b->pos.z;
        px2 = -nx0 * mul0 + b->pos.x;
        py2 = -ny0 * mul0 + b->pos.z;
        yw_MapLine(ywd->GfxObject, px0, py0, px1, py1, color);
        yw_MapLine(ywd->GfxObject, px2, py2, px1, py1, color);
    };

    /*** Advanced Info Layer, Overlay-Lifemeters zeichnen ***/
    if ((BCLID_YPAMISSY != b->BactClassID) &&
        (BCLID_YPAROBO  != b->BactClassID) &&
        (b->Owner == ywd->URBact->Owner))
    {
        if ((MR.layers & MAP_LAYER_HEIGHT) && (MR.zoom > 2)) {
            FLOAT nrg = ((FLOAT)b->Energy) / ((FLOAT)b->Maximum);
            if (nrg <= 0.25)      chr=128;
            else if (nrg <= 0.5)  chr=129;
            else if (nrg <= 0.75) chr=130;
            else                  chr=131;
            str = yw_MapChar(str,b->pos.x,b->pos.z,chr,char_width,char_height);
        };
    };
    
    /*** ENDE ***/
    return(str);
}

/*-----------------------------------------------------------------*/
void yw_MapTargetVecs(struct ypaworld_data *ywd, struct Bacterium *b)
/*
**  FUNCTION
**      if <b> Robo:
**          - Vector zum PrimTarget
**      else if <b> Commander:
**          - Vector zum PrimTarget oder (falls vorhanden)
**            zum SecTarget
**      else
**          - Vector zum Anführer
**          - Vector zum (falls vorhanden) SecTarget
**
**  CHANGED
**      14-Jun-96   floh    created
**                          + einheitliche Richtungs-Vektor
**                            Farben
**      30-Jul-96   floh    revised & updated (Dynamic Layout + Locale)
**      15-Sep-96   floh    revised & updated!
**      16-Feb-97   floh    + Bugfix: Es traten NULL-Pointer-Lesezugriffe
**                            auf, wenn b->master kein gültiges
**                            Objekt ist.
**      08-Oct-97   floh    + Waypoint-Handling
**      10-Oct-97   floh    + Sektor-Nebenziele werden jetzt auch
**                            als Linie gezeichnet.
**      09-Dec-97   floh    + yw_GetColor()
*/
{
    if ((b == ywd->URBact) || (b->master == b->robo)) {

        /*** Robo, oder ein Commander ***/
        if (TARTYPE_BACTERIUM == b->SecTargetType) {
            /*** SecTarget: Bacterium ***/
            yw_MapLine(ywd->GfxObject, b->pos.x, b->pos.z,
                       b->SecondaryTarget.Bact->pos.x,
                       b->SecondaryTarget.Bact->pos.z,
                       yw_GetColor(ywd,YPACOLOR_MAP_SECTARGET));
        } else if (TARTYPE_SECTOR == b->SecTargetType) {
            /*** SecTarget: Sektor ***/
            yw_MapLine(ywd->GfxObject, b->pos.x, b->pos.z,
                       b->SecPos.x, b->SecPos.z,
                       yw_GetColor(ywd,YPACOLOR_MAP_SECTARGET));
        } else {

            /*** Linie zum Prim-Target bzw. SecTarget ***/
            if (TARTYPE_SECTOR == b->PrimTargetType) {
                yw_MapLine(ywd->GfxObject, b->pos.x, b->pos.z,
                           b->PrimPos.x, b->PrimPos.z,
                           yw_GetColor(ywd,YPACOLOR_MAP_PRIMTARGET));
            } else if (TARTYPE_BACTERIUM == b->PrimTargetType) {
                yw_MapLine(ywd->GfxObject, b->pos.x, b->pos.z,
                           b->PrimaryTarget.Bact->pos.x,
                           b->PrimaryTarget.Bact->pos.z,
                           yw_GetColor(ywd,YPACOLOR_MAP_PRIMTARGET));
            };

            /*** wenn im Wegpunkt-Modus, Wegpunkte zeichnen ***/
            if (b->ExtraState & EXTRA_DOINGWAYPOINT) {
                ULONG i;
                FLOAT last_x,last_z;
                FLOAT next_x,next_z;
                if (b->ExtraState & EXTRA_WAYPOINTCYCLE) {
                    for (i=1; i<b->num_waypoints; i++) {
                        last_x = b->waypoint[i-1].x;
                        last_z = b->waypoint[i-1].z;
                        next_x = b->waypoint[i].x;
                        next_z = b->waypoint[i].z;
                        yw_MapLine(ywd->GfxObject,last_x,last_z,
                                   next_x,next_z, yw_GetColor(ywd,YPACOLOR_MAP_PRIMTARGET));
                    };
                    last_x = next_x; last_z = next_z;
                    next_x = b->waypoint[0].x;
                    next_z = b->waypoint[0].z;
                    yw_MapLine(ywd->GfxObject,last_x,last_z,
                               next_x,next_z, yw_GetColor(ywd,YPACOLOR_MAP_PRIMTARGET));
                } else {
                    if (b->num_waypoints > 0) {
                        for (i=b->count_waypoints; i<(b->num_waypoints-1); i++) {
                            last_x = b->waypoint[i].x;
                            last_z = b->waypoint[i].z;
                            next_x = b->waypoint[i+1].x;
                            next_z = b->waypoint[i+1].z;
                            yw_MapLine(ywd->GfxObject,last_x,last_z,
                                       next_x,next_z, yw_GetColor(ywd,YPACOLOR_MAP_PRIMTARGET));
                        };
                    };
                };
            };
        };

    } else {

        /*** ein Untergebener ***/
        if (TARTYPE_BACTERIUM == b->SecTargetType) {
            /*** Sekundär-Target? ***/
            yw_MapLine(ywd->GfxObject, b->pos.x, b->pos.z,
                       b->SecondaryTarget.Bact->pos.x,
                       b->SecondaryTarget.Bact->pos.z,
                       yw_GetColor(ywd,YPACOLOR_MAP_SECTARGET));
        } else {
            /*** Linie zum Vorgesetzten ***/
            if (b->master_bact) {
                yw_MapLine(ywd->GfxObject, b->pos.x, b->pos.z,
                           b->master_bact->pos.x, b->master_bact->pos.z,
                           yw_GetColor(ywd,YPACOLOR_MAP_COMMANDER));
            };
        };
    };
}

/*-----------------------------------------------------------------*/
void yw_MapDebugVecs(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Rendert für jeden autonomen Robo "Debug-Linien"
**      zur Visualisierung seiner Innereien.
**
**  CHANGED
**      06-Nov-96   floh    created
*/
{
    struct MinList *ls;
    struct MinNode *nd;

    ls = &(ywd->CmdList);
    for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {

        struct OBNode *obnd = (struct OBNode *)nd;
        struct Bacterium *rbact = obnd->bact;
        Object *ro = obnd->o;

        if ((BCLID_YPAROBO == rbact->BactClassID) && (rbact != ywd->URBact)) {

            /*** Zugriff auf LID des Robos ***/
            Class *rcl;
            struct yparobo_data *yrd;
            ULONG slot;

            _get(ro,OA_Class,&rcl);
            yrd = INST_DATA(rcl,ro);

            /*** für jeden Job-Slot des Robos ***/
            for (slot=0; slot<8; slot++) {
                ULONG pos = 0;
                switch(slot) {
                    case 0:
                        if (yrd->chk_Radar_Value > 0) pos=yrd->chk_Radar_Pos;
                        break;
                    case 1: 
                        if (yrd->chk_Safety_Value > 0) pos=yrd->chk_Safety_Pos;
                        break;
                    case 2: 
                        if (yrd->chk_Power_Value > 0) pos=yrd->chk_Power_Pos;
                        break;
                    case 3: 
                        if (yrd->chk_Enemy_Value > 0) pos=yrd->chk_Enemy_Pos;
                        break;
                    case 4: 
                        if (yrd->chk_Terr_Value > 0) pos=yrd->chk_Terr_Pos;
                        break;
                    case 5: 
                        if (yrd->chk_Place_Value > 0) pos=yrd->chk_Place_Pos;
                        break;
                    case 6: 
                        if (yrd->chk_Recon_Value > 0) pos=yrd->chk_Recon_Pos;
                        break;
                    case 7: 
                        if (yrd->chk_Robo_Value > 0) pos=yrd->chk_Robo_Pos;
                        break;
                };

                /*** Slot gueltig? ***/
                if (pos != 0) {
                    /*** dann Linie auf dessen Sektor-Pos ***/
                    LONG sec_x = pos % ywd->MapSizeX;
                    LONG sec_y = pos / ywd->MapSizeX;
                    FLOAT pos_x = sec_x*SECTOR_SIZE+(SECTOR_SIZE*0.5);
                    FLOAT pos_z = -(sec_y*SECTOR_SIZE+(SECTOR_SIZE*0.5));
                    /*** zeichne Linie ***/
                    yw_MapLine(ywd->GfxObject, rbact->pos.x, rbact->pos.z,
                               pos_x, pos_z,yw_GetColor(ywd,slot+YPACOLOR_OWNER_0));
                };
            };

            /*** jetzt blinkende Linie für Build- und Vehicle-Dock ***/
            if (0 != yrd->BuildSlot_Kind) {
                LONG sec_x = yrd->BuildSlot_Pos % ywd->MapSizeX;
                LONG sec_y = yrd->BuildSlot_Pos / ywd->MapSizeX;
                FLOAT pos_x = sec_x*SECTOR_SIZE+(SECTOR_SIZE*0.5);
                FLOAT pos_z = -(sec_y*SECTOR_SIZE+(SECTOR_SIZE*0.5));
                if ((ywd->TimeStamp / 300) & 1) {
                    yw_MapLine(ywd->GfxObject, rbact->pos.x, rbact->pos.z,
                               pos_x, pos_z,yw_GetColor(ywd,YPACOLOR_OWNER_0));
                };
            };

            if (0 != yrd->VehicleSlot_Kind) {
                LONG sec_x = yrd->VehicleSlot_Pos % ywd->MapSizeX;
                LONG sec_y = yrd->VehicleSlot_Pos / ywd->MapSizeX;
                FLOAT pos_x = sec_x*SECTOR_SIZE+(SECTOR_SIZE*0.5);
                FLOAT pos_z = -(sec_y*SECTOR_SIZE+(SECTOR_SIZE*0.5));
                if ((ywd->TimeStamp / 300) & 1) {
                    yw_MapLine(ywd->GfxObject, rbact->pos.x, rbact->pos.z,
                               pos_x, pos_z,yw_GetColor(ywd,YPACOLOR_OWNER_7));
                };
            };
        };
    };
}

/*-----------------------------------------------------------------*/
void yw_RenderMapVecs(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Rendert für ALLE Geschwader und dem Robo selbst des
**      Spielers den Target-Vector von der Bakterie bis zum
**      Ziel durch (clippt dabei natürlich gegen die
**      Map).
**
**  CHANGED
**      20-May-96   floh    created
**                          + falls ein Sekundär-Ziel vorliegt,
**                            wird dieses bevorzugt bearbeitet,
**                            außerdem wird beim Primär-Ziel jetzt
**                            unterschieden nach Sektor- und
**                            Bakterien-Ziel
**      14-Jun-96   floh    + im "Normal-Modus" nur noch eine
**                            Target-Linie vom ausgewählten Squad
**                            zu seinem momentan bearbeiteten
**                            Ziel.
**                          + einheitliche Richtungs-Vektor-Farben
**                          + Sonderfall im Radarmodus.
**      30-Jul-96   floh    revised & updated (Dynamic Layout + Locale)
**      07-Sep-96   floh    kein Target-Vektor für Robo mehr
**                          (der kann sich eh nicht mehr bewegen)
**      15-Sep-96   floh    revised & updated
**      06-Nov-96   floh    + Debug-Anzeige für autonome Robos, falls
**                            DebugInfo angeschaltet
**      11-Dec-96   floh    + wenn in User-Vehicle, wird SecTarget-
**                            Vektor nicht mehr angezeigt
**      16-Feb-97   floh    + Bugfix: yw_MapTargetVecs() konnte auf
**                            Commanders im ACTION_DEAD Zustand angewendet
**                            werden.
**      09-Dec-97   floh    + yw_GetColor()
**
*/
{
    if (MR.flags & MAPF_RADAR_MODE) {

        /*** nur, wenn nicht im Robo ***/
        if (ywd->UVBact != ywd->URBact) {

            /*** Richtung zum Vorgesetzten ***/
            yw_MapLine(ywd->GfxObject,
                       ywd->UVBact->pos.x, ywd->UVBact->pos.z,
                       ywd->UVBact->master_bact->pos.x,
                       ywd->UVBact->master_bact->pos.z,
                       yw_GetColor(ywd,YPACOLOR_MAP_COMMANDER));

            /*** falls Commander, Richtung zum PrimTarget ***/
            if (ywd->UVBact->master == ywd->UVBact->robo) {
                BOOL p_ok = FALSE;
                FLOAT x,z;
                if (TARTYPE_SECTOR==ywd->UVBact->PrimTargetType) {
                    x = ywd->UVBact->PrimPos.x;
                    z = ywd->UVBact->PrimPos.z;
                    p_ok = TRUE;
                } else if (TARTYPE_BACTERIUM==ywd->UVBact->PrimTargetType) {
                    x = ywd->UVBact->PrimaryTarget.Bact->pos.x;
                    z = ywd->UVBact->PrimaryTarget.Bact->pos.z;
                    p_ok = TRUE;
                };
                if (p_ok) {
                    yw_MapLine(ywd->GfxObject,
                               ywd->UVBact->pos.x, ywd->UVBact->pos.z,
                               x, z, yw_GetColor(ywd,YPACOLOR_MAP_PRIMTARGET));
                };
            };
        };

    } else {

        /*** EXTENDED MAP INFO LAYER? ***/
        if (MR.layers & MAP_LAYER_HEIGHT) {

            /*** Target-Vecs aller Commander ***/
            LONG i;
            for (i=0; i<ywd->NumCmdrs; i++) {

                struct MinList *ls;
                struct MinNode *nd;

                struct Bacterium *cmdr = ywd->CmdrRemap[i];
                if (ACTION_DEAD != cmdr->MainState) yw_MapTargetVecs(ywd,cmdr);

                /*** sowie aller Untergebener... ***/
                ls = &(cmdr->slave_list);
                for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {
                    struct Bacterium *b = ((struct OBNode *)nd)->bact;
                    if (ACTION_DEAD != b->MainState) yw_MapTargetVecs(ywd,b);
                };
            };

        } else {
            /*** nur das selektierte Geschwader ***/
            if (ywd->ActCmdr != -1) {
                struct Bacterium *b = ywd->CmdrRemap[ywd->ActCmdr];
                if (ACTION_DEAD != b->MainState) yw_MapTargetVecs(ywd,b);
            };
        };

        /*** Linien-Kreuz ueber Viewer ***/
        if (ywd->UVBact) {
            yw_MapLine(ywd->GfxObject, ywd->UVBact->pos.x, 0, ywd->UVBact->pos.x, -ywd->WorldSizeY,
                       yw_GetColor(ywd,YPACOLOR_MAP_VIEWER));
            yw_MapLine(ywd->GfxObject, 0, ywd->UVBact->pos.z, ywd->WorldSizeX, ywd->UVBact->pos.z,
                       yw_GetColor(ywd,YPACOLOR_MAP_VIEWER));
        }; 

        /*** Drag-Select-Box rendern, falls DragSelect underway ***/
        if (MR.flags & MAPF_DRAGGING) {
            struct rast_pens rp;
            struct rast_intline l0,l1,l2,l3;
            WORD cor_x = (ywd->DspXRes>>1);
            WORD cor_y = (ywd->DspYRes>>1);

            l0.x0=MR.drag_scr.xmin-cor_x; l0.x1=MR.drag_scr.xmax-cor_x;
            l1.x0=l0.x1; l1.x1=l0.x1;
            l2.x0=l0.x1; l2.x1=l0.x0;
            l3.x0=l0.x0; l3.x1=l0.x0;

            l0.y0=MR.drag_scr.ymin-cor_y; l0.y1=l0.y0;
            l1.y0=l0.y0; l1.y1=MR.drag_scr.ymax-cor_y;
            l2.y0=l1.y1; l2.y1=l1.y1;
            l3.y0=l1.y1; l3.y1=l1.y0;

            rp.fg_pen  = yw_GetColor(ywd,YPACOLOR_MAP_DRAGBOX);
            rp.fg_apen = yw_GetColor(ywd,YPACOLOR_MAP_DRAGBOX);
            rp.bg_pen  = -1;
            _methoda(ywd->GfxObject,RASTM_SetPens,&rp);
            _methoda(ywd->GfxObject,RASTM_IntClippedLine,&l0);
            _methoda(ywd->GfxObject,RASTM_IntClippedLine,&l1);
            _methoda(ywd->GfxObject,RASTM_IntClippedLine,&l2);
            _methoda(ywd->GfxObject,RASTM_IntClippedLine,&l3);
        };
    };

    /*** Debug-Anzeigen ***/
    if (ywd->DebugInfo) yw_MapDebugVecs(ywd);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_CursorsOverSquad(struct ypaworld_data *ywd,
                           UBYTE *str,
                           struct Bacterium *cmdr,
                           UBYTE font, UBYTE chr,
                           UBYTE width, UBYTE height)
/*
**  FUNCTION
**      Hebt alle Fahrzeuge des angegebenen Geschwaders
**      durch einen Cursor über jedem Fahrzeug hervor.
**
**  CHANGED
**      18-May-96   floh    created
**      20-May-96   floh    tote Bakterien werden jetzt ignoriert
**      27-May-96   floh    Fixes im Zshng mit AF's letztem Update
**      14-Jun-96   floh    revised & updated (volle Kontrolle über
**                          Font und dessen Size)
**      23-Jul-96   floh    Untergebene werden nur im Extended-Info-
**                          Layer gezeichnet.
**      30-Jul-96   floh    revised & updated (Dynamic Layout + Locale)
**      15-Sep-96   floh    revised & updated
*/
{
    struct MinList *ls;
    struct MinNode *nd;

    if (cmdr) {

        /*** Font-Init nur einmalig! ***/
        new_font(str,font);

        /*** erst der Squad-Leader ***/
        str = yw_MapChar(str,cmdr->pos.x,cmdr->pos.z,chr,width,height);

        /*** Slaves... ***/
        ls = &(cmdr->slave_list);
        for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {

            struct Bacterium *b = ((struct OBNode *)nd)->bact;

            if (ACTION_DEAD != b->MainState) {
                str = yw_MapChar(str,b->pos.x,b->pos.z,chr,width,height);
            };
        };
    };


    /*** Ende ***/
    return(str);
}

#ifdef YPA_DESIGNMODE
/*-----------------------------------------------------------------*/
UBYTE *yw_RenderMapKoords(struct ypaworld_data *ywd, UBYTE *str)
/*
**  FUNCTION
**      Rendert ein paar Koords in die Map.
**
**  CHANGED
**      24-May-96   floh    created
**      30-Jul-96   floh    revised & updated (Dynamic Layout + Locale)
**      15-Sep-96   floh    revised & updated
*/
{
    if (ywd->FrameFlags & YWFF_MouseOverSector) {
        new_font(str,FONTID_TRACY);
        pos_abs(str,MR.topleft_x,MR.topleft_y);

        /*** und die Strings... ***/
        str += sprintf(str, "[%d,%d]", ywd->SelSecX, ywd->SelSecY);
        new_line(str);
        str += sprintf(str, "[%d,%d]",
                       (LONG)ywd->SelSecIPos.x,
                       (LONG)ywd->SelSecIPos.z);
    };

    return(str);
}
#endif

/*-----------------------------------------------------------------*/
UBYTE *yw_OverlayCursor(struct ypaworld_data *ywd,
                        UBYTE *str,
                        ULONG sec_x, ULONG sec_y,
                        ULONG sec_size, UBYTE chr)
/*
**  FUNCTION
**      Rendert einen Overlay-Cursor an die angegebene
**      Sektor-Position.
**
**  CHANGED
**      16-Feb-98   floh    created
*/
{
    FLOAT smx = (FLOAT) (sec_x * SECTOR_SIZE + SECTOR_SIZE/2);
    FLOAT smz = (FLOAT) -(sec_y * SECTOR_SIZE + SECTOR_SIZE/2);
    return(yw_MapChar(str,smx,smz,chr,sec_size,sec_size));
}

/*-----------------------------------------------------------------*/
UBYTE *yw_RenderOverlayCursors(struct ypaworld_data *ywd, UBYTE *str)
/*
**  FUNCTION
**      Rendert die Overlay-Cursors über diverse wichtige
**      Sektoren.
**
**  CHANGED
**      17-Jul-97   floh    created
**      16-Feb-98   floh    + Beamgate Overlays
*/
{
    /*** FontID und Size für Sector-Cursors ***/
    ULONG sec_fid,sec_size;
    ULONG i;

    switch(MR.zoom) {
        case 0:
            sec_fid    = FONTID_MAPCUR_4;
            sec_size   = 4;
            break;
        case 1:
            sec_fid    = FONTID_MAPCUR_8;
            sec_size   = 8;
            break;
        case 2:
            sec_fid    = FONTID_MAPCUR_16;
            sec_size   = 16;
            break;
        case 3:
            sec_fid    = FONTID_MAPCUR_32;
            sec_size   = 32;
            break;
        case 4:
            sec_fid    = FONTID_MAPCUR_64;
            sec_size   = 64;
            break;
    };

    /*** Overlays in den oberen Zoomstufen und Landscape Layer an ***/
    if ((sec_size >= 8) && (MR.layers & MAP_LAYER_LANDSCAPE)) {

        new_font(str,sec_fid);

        /*** Overlay Cursor über alle Kraftwerke ***/
        for (i=0; i<ywd->FirstFreeKraftWerk; i++) {
            struct KraftWerk *kw = &(ywd->KraftWerks[i]);

            /*** Sektor muß im Footprint-Bereich sein! ***/
            if ((kw->sector) &&
                (kw->sector->FootPrint & (1<<ywd->URBact->Owner)))
            {
                ULONG power = kw->factor;
                if (power > 0) {
                    UBYTE chr   = 0;
                    FLOAT smx = (FLOAT) (kw->x * SECTOR_SIZE + SECTOR_SIZE/2);
                    FLOAT smz = (FLOAT) -(kw->y * SECTOR_SIZE + SECTOR_SIZE/2);
                    if      (power <= 32)   chr = 128;
                    else if (power <= 64)   chr = 129;
                    else if (power <= 96)   chr = 130;
                    else if (power <= 128)  chr = 131;
                    else if (power <= 160)  chr = 132;
                    else if (power <= 192)  chr = 133;
                    else if (power <= 224)  chr = 134;
                    else if (power <= 256)  chr = 135;
                    /*** zuerst das Kraftwerks-Symbol ***/
                    str = yw_MapChar(str,smx,smz,137,sec_size,sec_size);
                    str = yw_MapLenVertChar(str,smx,smz,chr,sec_size,sec_size>>3);
                };
            };
        };

        /*** ... Wundersteine ***/
        for (i=0; i<MAXNUM_WUNDERSTEINS; i++) {
            struct Wunderstein *gem = &(ywd->gem[i]);
            struct Cell *sec = &(ywd->CellArea[gem->sec_y*ywd->MapSizeX+gem->sec_x]);
            if ((gem->active) && (sec->FootPrint & (1<<ywd->URBact->Owner)))
            {
                BOOL do_gem = TRUE;
                if (sec->WType == WTYPE_Wunderstein) {
                    /*** Wunderstein wurde noch nicht erobert, also blinken lassen ***/
                    if ((ywd->TimeStamp/300) & 1) do_gem=FALSE;
                };
                if (do_gem) str = yw_OverlayCursor(ywd,str,gem->sec_x,gem->sec_y,sec_size,136);
            };
        };

        /*** Beamgates mit Keysektoren ***/
        for (i=0; i<ywd->Level->NumGates; i++) {
            struct Gate *g = &(ywd->Level->Gate[i]);
            if (WTYPE_ClosedGate == g->sec->WType) {
                ULONG j;

                /*** Beamgate (geschlossen) ***/
                if (g->sec->FootPrint & (1<<ywd->URBact->Owner)) {
                    str = yw_OverlayCursor(ywd,str,g->sec_x,g->sec_y,sec_size,147);
                };

                /*** und die Keysektoren ***/
                for (j=0; j<g->num_keysecs; j++) {
                    struct Cell *sec = g->keysec[j].sec;
                    ULONG sec_x = g->keysec[j].sec_x;
                    ULONG sec_y = g->keysec[j].sec_y;
                    BOOL do_keysec = FALSE;
                    if (sec && (sec->FootPrint & (1<<ywd->URBact->Owner))) {
                        if (sec->Owner == ywd->URBact->Owner) do_keysec = TRUE;
                        else if ((ywd->TimeStamp / 300) & 1)  do_keysec = TRUE;
                    };
                    if (do_keysec) {                     
                        str = yw_OverlayCursor(ywd,str,sec_x,sec_y,sec_size,138);
                    };
                };
            } else if (WTYPE_OpenedGate == g->sec->WType) {
                /*** Beamgate offen ***/
                if ((g->sec->FootPrint & (1<<ywd->URBact->Owner) &&
                    (ywd->TimeStamp / 300) & 1))
                {
                    str = yw_OverlayCursor(ywd,str,g->sec_x,g->sec_y,sec_size,148);
                };
            };
        };

        /*** Superitems mit Keysektoren ***/
        for (i=0; i<ywd->Level->NumItems; i++) {

            struct SuperItem *item = &(ywd->Level->Item[i]);
            UBYTE item_chr   = 0;
            UBYTE keysec_chr = 0;

            /*** welches Char? ***/
            switch(item->type) {
                case SI_TYPE_BOMB:
                    keysec_chr = 142;
                    switch(item->status) {
                        case SI_STATUS_INACTIVE:
                            item_chr = 139;
                            break;
                        case SI_STATUS_ACTIVE:
                        case SI_STATUS_FROZEN:
                            item_chr = 140;
                            break;
                        case SI_STATUS_TRIGGERED:
                            item_chr = 141;
                            break;
                    };
                    break;
                case SI_TYPE_WAVE:
                    keysec_chr = 146;
                    switch(item->status) {
                        case SI_STATUS_INACTIVE:
                            item_chr = 143;
                            break;
                        case SI_STATUS_ACTIVE:
                        case SI_STATUS_FROZEN:
                            item_chr = 144;
                            break;
                        case SI_STATUS_TRIGGERED:
                            item_chr = 145;
                            break;
                    };
                    break;
            };

            /*** ein gültiger Typ und Zustand? ***/
            if (item_chr) {
                /*** Superitem... falls feindlich, blinkt es, ansonsten statisch ***/
                BOOL do_item = FALSE;
                if (item->sec->FootPrint & (1<<ywd->URBact->Owner)) {
                    if (item->sec->Owner == ywd->URBact->Owner) do_item=TRUE;
                    else if ((ywd->TimeStamp / 300) & 1)        do_item=TRUE;
                };
                if (do_item) {                             
                    str = yw_OverlayCursor(ywd,str,item->sec_x,item->sec_y,
                                           sec_size,item_chr);
                };
                
                if (keysec_chr) {
                    ULONG j;
                    for (j=0; j<item->num_keysecs; j++) {
                        struct Cell *sec = item->keysec[j].sec;
                        ULONG sec_x = item->keysec[j].sec_x;
                        ULONG sec_y = item->keysec[j].sec_y;
                        ULONG do_keysec = FALSE;
                        if (sec && (sec->FootPrint & (1<<ywd->URBact->Owner))) {
                            if (sec->Owner == ywd->URBact->Owner) do_keysec = TRUE;
                            else if ((ywd->TimeStamp / 500) & 1)  do_keysec = TRUE;                               
                        };
                        if (do_keysec) {
                            str = yw_OverlayCursor(ywd,str,sec_x,sec_y,sec_size,item_chr);
                        };
                    };
                };

            };
        };
    };
    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_RenderMapCursors(struct ypaworld_data *ywd, UBYTE *str)
/*
**  FUNCTION
**      Ziemlich komplexe Routine, die die Zusammenhänge auf der
**      Map mit Hilfe von zusätzlichen Cursors darstellt.
**      Die Routine baut auf die Selektions-Ergebnisse
**      in yw_BuildTrLogicMsg() auf, also sicherstellen, daß
**      sie innerhalb des Frames danach aufgerufen wird
**      (wird zur Zeit sichergestellt durch den PostDrawHook
**      des Map-Vehikel-Renderings).
**
**  INPUTS
**      ywd - ...
**      str - Output-Stream
**      topleft_x - X-Koord der linken oberen Map-Ecke im Screen
**      topleft_y - ditto, y
**
**  RESULTS
**      str - modifizierter Output-Stream
**
**  CHANGED
**      17-May-96   floh    created
**      19-May-96   floh    Bei GOTO-Feindfahrzeug wird nicht mehr
**                          das gesamte feindliche Geschwader gehighlightet,
**                          sondern nur noch das angewählte Fahrzeug.
**      23-May-96   floh    erweitert um Designer-Stuff...
**      24-May-96   floh    Panic-Mode removed
**      25-May-96   floh    ruft im Designer-Modus yw_RenderMapKoords()
**                          auf
**      14-Jun-96   floh    Revised & Updated
**                          + bei "Maus über feindlichem Sektor"
**                            zuerst Footprint-Test, dann Owner-Test
**      22-Jul-96   floh    YW_ACTION_FIRE Behandlung gekillt
**      30-Jul-96   floh    revised & updated (Dynamic Layout + Locale)
**      07-Sep-96   floh    Falls Zielpunkt des selektiertem Geschwaders
**                          in einem feindlichen Sektor, wird zusätzlich
**                          zum Attack-Sektor-Cursor ein Ortscursor
**                          über dem präzisen Zielpunkt gezeichnet.
**      15-Sep-96   floh    revised & updated
**      12-Dec-96   floh    Support für Drag-Select in Map.
**      27-May-97   floh    Cursor-Chars moved
**      03-Jun-97   floh    + Zoomstufe 4
**      03-Jul-97   floh    + Overlay-Cursors über Kraftwerke und Wundersteine
**      05-Jul-97   floh    + Char-Codes wurden verlagert, um mit
**                            dem TypeNS-Font übereinzustimmen
**      15-Dec-97   floh    + Bugfix: Robo possibly selected war broken
**                          + Bugfix: Robo selected war auch broken
**      20-May-98   floh    + LastMessage-Sender-Anzeige war broken
**      21-May-98   floh    + sollte jetzt ueber allen Wegpunkten des
**                            ausgewaehlten Geschwaders einen Orts-Cursor
**                            zeichnen.
**      13-Jun-98   floh    + Fahrzeug-Anzahl ueber offene Beamgates
*/
{
    /*** FontID und Size für Sector-Cursors ***/
    ULONG sec_fid,vhc_fid,sec_size,vhc_width,vhc_height;
    ULONG robo_char,robo_selchar,robo_posselchar;
    ULONG i;
    struct Bacterium *lm_bact;

    switch(MR.zoom) {
        case 0:
            sec_fid    = FONTID_MAPCUR_4;
            sec_size   = 4;
            vhc_fid    = FONTID_BACT3X3;
            vhc_height = ywd->Fonts[vhc_fid]->height;
            vhc_width  = ywd->Fonts[vhc_fid]->fchars[1].width;
            robo_char       = MAP_CHAR_ROBO_SMALL;
            robo_selchar    = MAP_CHAR_RSEL_SMALL;
            robo_posselchar = MAP_CHAR_RPOSSEL_SMALL;
            break;

        case 1:
            sec_fid    = FONTID_MAPCUR_8;
            sec_size   = 8;
            vhc_fid    = FONTID_BACT3X3;
            vhc_height = ywd->Fonts[vhc_fid]->height;
            vhc_width  = ywd->Fonts[vhc_fid]->fchars[1].width;
            robo_char       = MAP_CHAR_ROBO_SMALL;
            robo_selchar    = MAP_CHAR_RSEL_SMALL;
            robo_posselchar = MAP_CHAR_RPOSSEL_SMALL;
            break;

        case 2:
            sec_fid    = FONTID_MAPCUR_16;
            sec_size   = 16;
            vhc_fid    = FONTID_BACT5X5;
            vhc_height = ywd->Fonts[vhc_fid]->height;
            vhc_width  = ywd->Fonts[vhc_fid]->fchars[1].width;
            robo_char       = MAP_CHAR_ROBO_MEDIUM;
            robo_selchar    = MAP_CHAR_RSEL_MEDIUM;
            robo_posselchar = MAP_CHAR_RPOSSEL_MEDIUM;
            break;

        case 3:
            sec_fid    = FONTID_MAPCUR_32;
            sec_size   = 32;
            vhc_fid    = FONTID_BACT7X7;
            vhc_height = ywd->Fonts[vhc_fid]->height;
            vhc_width  = ywd->Fonts[vhc_fid]->fchars[1].width;
            robo_char       = MAP_CHAR_ROBO_BIG;
            robo_selchar    = MAP_CHAR_RSEL_BIG;
            robo_posselchar = MAP_CHAR_RPOSSEL_BIG;
            break;

        case 4:
            sec_fid    = FONTID_MAPCUR_64;
            sec_size   = 64;
            vhc_fid    = FONTID_TYPE_NS; // FONTID_BACT9X9;
            vhc_height = ywd->Fonts[vhc_fid]->height;
            vhc_width  = ywd->Fonts[vhc_fid]->fchars[1].width;
            robo_char       = MAP_CHAR_ROBO_HUGE;
            robo_selchar    = MAP_CHAR_RSEL_HUGE;
            robo_posselchar = MAP_CHAR_RPOSSEL_HUGE;
            break;
    };

    /*** erstmal die Action-Spezifischen Sachen... ***/
    switch(ywd->Action) {
        case YW_ACTION_SELECT:
            /*** nur, wenn nicht DragSelecting ***/
            if (!(MR.flags & MAPF_DRAGGING)) {
                if (ywd->SelBact && (ywd->SelBact != ywd->URBact)) {
                    struct Bacterium *sel = yw_GetCommander(ywd->SelBact);
                    if (sel) {
                        /*** ein normales Squad... ***/
                        str = yw_CursorsOverSquad(ywd, str,
                              sel, vhc_fid, MAP_CURSOR_POSSEL,
                              vhc_width, vhc_height);
                    } else if (ywd->SelBact == ywd->URBact) {
                        /*** Sonderfall Robo ***/
                        ULONG size = ywd->Fonts[MAP_FONT_ROBO]->height;
                        str = yw_MapFontChar(str,MAP_FONT_ROBO,
                                         ywd->SelBact->pos.x, ywd->SelBact->pos.z,
                                         robo_posselchar, size, size);
                    };
                };
            };
            break;

        case YW_ACTION_GOTO:
            /*** nur, wenn nicht DragSelecting ***/
            if (!(MR.flags & MAPF_DRAGGING)) {
                if (ywd->FrameFlags & YWFF_MouseOverBact) {
                    /*** Maus ist ueber einem Feindfahrzeug ***/
                    str = yw_MapFontChar(str,vhc_fid,
                                     ywd->SelBact->pos.x,
                                     ywd->SelBact->pos.z,
                                     MAP_CURSOR_POSTAR,
                                     vhc_width, vhc_height);

                } else if (ywd->FrameFlags & YWFF_MouseOverSector) {
                    if ((ywd->SelSector->Owner == ywd->URBact->Owner) &&
                        (ywd->SelSector->FootPrint & (1<<ywd->URBact->Owner)))
                    {
                        /*** ein eigener Sektor -> Ortscursor zeichnen ***/
                        str = yw_MapFontChar(str, vhc_fid,
                                             ywd->SelSecIPos.x,
                                             ywd->SelSecIPos.z,
                                             MAP_CURSOR_GOTO,
                                             vhc_width, vhc_height);
                    } else {
                        /*** ein feindlicher Sektor -> Attack-Cursor zeichnen ***/
                        str = yw_MapFontChar(str, sec_fid,
                                             ywd->SelSecMPos.x,
                                             ywd->SelSecMPos.z,
                                             'B', sec_size, sec_size);
                    };
                };
            };
            break;

        case YW_ACTION_CONTROL:
            /*** Cursor über "Ziel"-Vehikel ***/
            str = yw_MapFontChar(str, vhc_fid,
                                 ywd->SelBact->pos.x,
                                 ywd->SelBact->pos.z,
                                 MAP_CURSOR_POSSEL,
                                 vhc_width, vhc_height);
            break;

        case YW_ACTION_BUILD:
            /*** Sektor-Cursor über Ziel-Sektor ***/
            str = yw_MapFontChar(str, sec_fid,
                                 ywd->SelSecMPos.x,
                                 ywd->SelSecMPos.z,
                                 'A', sec_size, sec_size);
            break;

        case YW_ACTION_APILOT:
            /*** Ortscursor... ***/
            str = yw_MapFontChar(str, vhc_fid,
                                 ywd->SelSecIPos.x,
                                 ywd->SelSecIPos.z,
                                 MAP_CURSOR_GOTO,
                                 vhc_width, vhc_height);
            break;

        #ifdef YPA_DESIGNMODE
        case YW_ACTION_SETSECTOR:
        case YW_ACTION_SETOWNER:
        case YW_ACTION_SETHEIGHT:
            /*** Sektor-Cursor ***/
            str = yw_MapFontChar(str, sec_fid,
                                 ywd->SelSecMPos.x,
                                 ywd->SelSecMPos.z,
                                 'A', sec_size, sec_size);
            break;
        #endif
    };

    /*** selektiertes Geschwader ***/
    if (SR.ActiveMode & STAT_MODEF_AUTOPILOT) {

        /*** Beam-Modus, Robo ausgewaehlt ***/
        struct Bacterium *r = ywd->URBact;
        ULONG size = ywd->Fonts[MAP_FONT_ROBO]->height;
        str = yw_MapFontChar(str,MAP_FONT_ROBO,r->pos.x,r->pos.z,robo_selchar,size,size);
    
    } else if (ywd->ActCmdr != -1) {
        struct Bacterium *cmdr;
        if (cmdr = ywd->CmdrRemap[ywd->ActCmdr]) {

            /*** das selektierte Geschwader hervorheben... ***/
            str = yw_CursorsOverSquad(ywd,str,cmdr,vhc_fid,MAP_CURSOR_SELECTED,vhc_width,vhc_height);

            /*** ...und sein momentanes Ziel ***/
            switch (cmdr->PrimTargetType) {

                case TARTYPE_SECTOR:
                    if (cmdr->PrimaryTarget.Sector->Owner == ywd->URBact->Owner) {
                        /*** ein eigener Sektor -> Ortscursor ***/
                        str = yw_MapFontChar(str, vhc_fid,
                                             cmdr->PrimPos.x,
                                             cmdr->PrimPos.z,
                                             MAP_CURSOR_GOTO,
                                             vhc_width, vhc_height);
                    } else {

                        /*** ein feindlicher Sektor -> Sektor-Cursor ***/

                        /*** die Rück-Ermittlung des Sektor-Mittelpunkts ***/
                        /*** ist leider nicht ganz trivial...            ***/
                        LONG secx = ((LONG)(cmdr->PrimPos.x))/((WORD)SECTOR_SIZE);
                        LONG secy = ((LONG)(-cmdr->PrimPos.z))/((WORD)SECTOR_SIZE);

                        FLOAT smx = (FLOAT) (secx * SECTOR_SIZE + SECTOR_SIZE/2);
                        FLOAT smz = (FLOAT) -(secy * SECTOR_SIZE + SECTOR_SIZE/2);
                        str = yw_MapFontChar(str,sec_fid,smx,smz,'B',sec_size,sec_size);

                        /*** zusätzlicher Ortscursor über "echtem" Zielpunkt ***/
                        str = yw_MapFontChar(str, vhc_fid,
                                             cmdr->PrimPos.x,
                                             cmdr->PrimPos.z,
                                             MAP_CURSOR_GOTO,
                                             vhc_width, vhc_height);
                    };
                    break;

                case TARTYPE_BACTERIUM:

                    /*** einen Cursor auf das Target-Bacterium ***/
                    str = yw_MapFontChar(str, vhc_fid,
                                         cmdr->PrimaryTarget.Bact->pos.x,
                                         cmdr->PrimaryTarget.Bact->pos.z,
                                         MAP_CURSOR_POSTAR,
                                         vhc_width, vhc_height);
                    break;
            };
            
            /*** wenn im Wegpunkt-Modus, Wegpunkte zeichnen ***/
            if (cmdr->ExtraState & EXTRA_DOINGWAYPOINT) {
                ULONG i;
                if (cmdr->num_waypoints > 0) {
                    if (cmdr->ExtraState & EXTRA_WAYPOINTCYCLE) i=0;
                    else                                        i=cmdr->count_waypoints;
                    for (i; i<cmdr->num_waypoints; i++) {
                        str = yw_MapFontChar(str, vhc_fid, 
                                cmdr->waypoint[i].x,
                                cmdr->waypoint[i].z,
                                MAP_CURSOR_GOTO,
                                vhc_width, vhc_height);
                    };
                };
            };
        };
    };

    /*** Blinkender Cursor über User-Vehikel ***/
    if (ywd->UVBact) {
        if ((ywd->TimeStamp / 300) & 1) {
            str = yw_MapFontChar(str, vhc_fid,
                    ywd->UVBact->pos.x,
                    ywd->UVBact->pos.z,
                    MAP_CURSOR_VIEWER,
                    vhc_width, vhc_height);
        };
    };

    /*** Blinkender LastMessage Cursor ***/
    if (lm_bact = ywd->LastMessageSender) {
        if ((ywd->TimeStamp / 300) & 1) {
            str = yw_MapFontChar(str,vhc_fid, lm_bact->pos.x, lm_bact->pos.z,
                  MAP_CURSOR_LASTMESSAGE, vhc_width, vhc_height);
        };
    };
    
    /*** Fahrzeug-Anzahl ueber offene Beamgates ***/
    for (i=0; i<ywd->Level->NumGates; i++) {
        struct Gate *g = &(ywd->Level->Gate[i]);
        if (WTYPE_OpenedGate == g->sec->WType) {
            LONG b_count = yw_CountVehiclesInSector(ywd,g->sec);
            UBYTE buf[32];
            sprintf(buf,"%d/%d",b_count,ywd->Level->MaxNumBuddies);
            str = yw_MapString(ywd,buf,FONTID_LTRACY,str,g->sec_x,g->sec_y);
        };
    };
    
    #ifdef YPA_DESIGNMODE
    str = yw_RenderMapKoords(ywd,str);
    #endif

    /*** Ende ***/
    return(str);
}

/*-----------------------------------------------------------------*/
void yw_RenderMapVehicles(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Being the Hook, der die Vehicles drawed...
**      Nur für die "echte" Map benutzen! Das Radar
**      benötigt eine extra Routine.
**
**  CHANGED
**      21-Nov-95   floh    created
**      23-Nov-95   floh    debugging...
**      29-Dec-95   floh    more debugging...
**      06-Feb-96   floh    + alle Bakterien jetzt 5 Pixel, außerdem
**                            wird in allen Zoom-Stufen gezeichnet.
**                          + Alle Bakterien werden in einen Char-Buffer
**                            gehauen (dem eigentlichen Map-Buffer,
**                            weil der schön groß und inzwischen frei
**                            ist) und in einem Rutsch gezeichnet
**                            (per _DrawText()). Das hat außerdem den
**                            angenehmen Neben-Effekt, daß die Richtungs-
**                            Vektoren von Bakterien übermalt werden.
**      13-Mar-96   floh    revised & updated (neues Map-Layout)
**      21-Mar-96   floh    beachtet jetzt Sektor-Footprint.
**      20-May-96   floh    + yw_RenderTargetVecs()
**                          + Font-Größen für Map-Vehikel jetzt 3->5->7
**      26-Jun-96   floh    revised & updated für Map kleiner als Window
**      30-Jul-96   floh    revised & updated (Dynamic Layout + Locale)
**      15-Sep-96   floh    revised & updated
**      12-Dec-96   floh    ein zweiter Durchgang highlited die
**                          die Bakterien innerhalb der Select-Drag-
**                          Box (wenn Drag-Select überhaupt aktiv ist!)
**      27-May-97   floh    + Typ-spezifische Vehicle-Brushes
**      29-May-97   floh    + yw_MRLayoutMapButtons()
**      05-Jul-97   floh    + in größter Zoomstufe wieder Vehikel-Icons
**      17-Jul-97   floh    + Overlay-Cursors werden jetzt vor den Vehicles
**                            gerendert
*/
{
    LONG sz_shift;
    LONG sec_x, sec_y, end_x, end_y;
    LONG sx,sy;
    UWORD char_width,char_height;
    UBYTE font_id;
    struct MinList *ls;
    struct MinNode *nd;
    struct drawtext_args dt_args;
    UBYTE *str;
    BOOL do_type_icon = FALSE;
    struct rast_intrect clip;
    ULONG num_chars = 0;

    /*** DrawText-Args-Struktur initialisieren ***/
    dt_args.string = MR_ClipRealMap;
    dt_args.clips  = NULL;
    str = dt_args.string;

    /*** diverse Params ermitteln (Achtung, Hack) ***/
    sz_shift  = MR.zoom + 2;    // 2..6 (== 4..64 Pixel pro Sektor)
    switch(MR.zoom) {
        case 0:
        case 1:
            font_id = FONTID_BACT3X3;
            break;

        case 2:
            font_id = FONTID_BACT5X5;
            break;

        case 3:
            font_id = FONTID_BACT7X7;
            break;

        case 4:
            // font_id = FONTID_BACT9X9;
            font_id = FONTID_TYPE_NS;
            do_type_icon = TRUE;
            break;
    };

    /*** Fontgröße direkt rausholen ***/
    char_width  = ywd->Fonts[font_id]->fchars[128].width;
    char_height = ywd->Fonts[font_id]->height;

    MR.pw_x = MR.req.req_cbox.rect.w - MR.BorHori,
    MR.pw_y = MR.req.req_cbox.rect.h - MR.BorVert,
    MR.ps_x =  (MR.midx / MR.x_aspect) - (MR.pw_x>>1);
    MR.ps_y = -(MR.midz / MR.y_aspect) - (MR.pw_y>>1);
    MR.topleft_x = (MR.req.req_cbox.rect.x + MR.BorLeftW) - (ywd->DspXRes>>1);
    MR.topleft_y = (MR.req.req_cbox.rect.y + MR.BorTopH)  - (ywd->DspYRes>>1);

    /*** setze ClipRect für Linien ***/
    clip.xmin = MR.topleft_x;
    clip.ymin = MR.topleft_y;
    clip.xmax = MR.topleft_x + MR.pw_x - 1;
    clip.ymax = MR.topleft_y + MR.pw_y - 1;
    _methoda(ywd->GfxObject, RASTM_IntClipRegion, &clip);

    /*** Start-Sektor und End-Sektor ermitteln ***/
    sec_x = MR.ps_x >> sz_shift;
    sec_y = MR.ps_y >> sz_shift;
    end_x = (MR.ps_x + MR.pw_x) >> sz_shift;
    end_y = (MR.ps_y + MR.pw_y) >> sz_shift;

    if (sec_x < 1) sec_x = 1;
    if (sec_y < 1) sec_y = 1;
    if (end_x >= ywd->MapSizeX) end_x = ywd->MapSizeX-1;
    if (end_y >= ywd->MapSizeY) end_y = ywd->MapSizeY-1;

    /*** zuerst die Overlay-Cursors ***/
    str = yw_RenderOverlayCursors(ywd,str);

    /*** Vehicle Font initialisieren ***/
    new_font(str,font_id);

    /*** erstmal die TargetVectoren für ALLE Geschwader ***/
    yw_RenderMapVecs(ywd);

    /*** Runde 1: die Roh-Bakterien ***/
    for (sy = sec_y; sy <= end_y; sy++) {
        for (sx = sec_x; sx <= end_x; sx++) {
            if (num_chars < 2048) {
            
                struct Cell *sec = &(ywd->CellArea[sy * ywd->MapSizeX + sx]);

                /*** für jede Bakterie im Sektor... ***/
                if (sec->FootPrint & MR.footprint) {
                    ls = (struct MinList *) &(sec->BactList);
                    for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {
                        struct Bacterium *b = (struct Bacterium *) nd;
                        if (BCLID_YPAROBO == b->BactClassID) {
                            /*** Robo-Sonderbehandlung ***/
                            new_font(str,MAP_FONT_ROBO);
                            str = yw_RenderMapBact(ywd,str,
                                  ywd->Fonts[MAP_FONT_ROBO]->height,
                                  ywd->Fonts[MAP_FONT_ROBO]->fchars[MAP_CHAR_ROBO_HUGE].width,
                                  b, FALSE);
                            new_font(str,font_id);
                            num_chars++;
                        } else {
                            /*** Robo-Flaks ignorieren ***/
                            if (BCLID_YPAGUN == b->BactClassID) {
                                /*** isses eine Robo-Flak? ***/
                                struct ypagun_data *yd;
                                yd = INST_DATA( ((struct nucleusdata *)b->BactObject)->o_Class,b->BactObject);
                                if (!(yd->flags & GUN_RoboGun)) {
                                    /*** es isset keine, also rendern ***/
                                    str = yw_RenderMapBact(ywd,str,char_width,
                                    char_height,b,do_type_icon);
                                    num_chars++;
                                };
                            } else {
                                str = yw_RenderMapBact(ywd,str,char_width,
                                char_height,b,do_type_icon);
                                num_chars++;
                            };
                        };
                    };
                };
            };
        };
    };

    /*** Runde 2: Dragbox-Highlights ***/
    if (MR.flags & MAPF_DRAGGING) {
        for (sy = sec_y; sy <= end_y; sy++) {
            for (sx = sec_x; sx <= end_x; sx++) {
                if (num_chars < 2048) {
                    struct Cell *sec = &(ywd->CellArea[sy * ywd->MapSizeX + sx]);

                    /*** für jede Bakterie im Sektor... ***/
                    if (sec->FootPrint & MR.footprint) {
                        ls = (struct MinList *) &(sec->BactList);
                        for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {

                            struct Bacterium *b = (struct Bacterium *)nd;

                            /*** die Auswahl-Kriterien sind ziemlich komplex... ***/
                            if ((b->Owner == ywd->URBact->Owner)   &&
                                (b->pos.x > MR.drag_world.xmin)    &&
                                (b->pos.z > MR.drag_world.ymin)    &&
                                (b->pos.x < MR.drag_world.xmax)    &&
                                (b->pos.z < MR.drag_world.ymax)    &&
                                (b->BactClassID != BCLID_YPAMISSY) &&
                                (b->BactClassID != BCLID_YPAROBO)  &&
                                (b->BactClassID != BCLID_YPAGUN)   &&
                                (b->MainState   != ACTION_CREATE)  &&
                                (b->MainState   != ACTION_BEAM)    &&
                                (b->MainState   != ACTION_DEAD))
                            {
                                /*** wow, ein Treffer ***/
                                str = yw_MapChar(str,b->pos.x,b->pos.z,
                                MAP_CURSOR_POSSEL,char_width,char_height);
                                num_chars++;
                            };
                        };
                    };
                };
            };
        };
    };

    /*** diversigste weitere Cursors... ***/
    str = yw_RenderMapCursors(ywd, str);

    /*** Map-Buttons rendern ***/
    str = yw_MRLayoutMapButtons(ywd, str);

    /*** EOS setzen und zeichnen ***/
    eos(str);
    _DrawText(&dt_args);

    /*** yo, africaan people, unite! ***/
}

/*-----------------------------------------------------------------*/
void yw_RenderRadarVehicles(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Variation von yw_RenderMapVehicles(), speziell
**      für das neue Rotations-Radar.
**
**  CHANGED
**      15-Sep-96   floh    created
**      23-Sep-96   floh    die zu bearbeitenden Sektoren müßten
**                          jetzt korrekt sein (Rundungsproblem)
**      30-Apr-97   floh    keine Ziel-Linien mehr im Radar
**      11-Dec-97   floh    falls Map offen, machen Radar-Vehikel einen
**                          Verdeckungstest gegen die Map
**      17-Dec-97   floh    + yw_3DCursorOverBact()
**      04-May-98   floh    + Radar rendert jetzt nur noch max.
**                            512 Vehikel, um einen Bufferueberlauf zu 
**                            verhindern
*/
{
    LONG sec_x, sec_y, end_x, end_y, store_sec_x;
    UWORD char_width,char_height;
    UBYTE font_id;
    struct MinList *ls;
    struct MinNode *nd;
    struct drawtext_args dt_args;
    UBYTE *str;
    BOOL do_type_icon = FALSE;
    struct rast_intrect clip;
    ULONG num_bacts;

    /*** DrawText-Args-Struktur initialisieren ***/
    dt_args.string = RadarStr;
    dt_args.clips  = NULL;

    str = dt_args.string;

    /*** diverse Params ermitteln (Achtung, Hack) ***/
    char_width = char_height = 7;
    font_id   = FONTID_BACT7X7;

    /*** Font initialisieren ***/
    new_font(str,font_id);

    MR.pw_x = FLOAT_TO_INT(((FLOAT)(ywd->DspXRes>>1)) * (MR.r_x1-MR.r_x0));
    MR.pw_y = FLOAT_TO_INT(((FLOAT)(ywd->DspYRes>>1)) * (MR.r_y1-MR.r_y0));
    MR.ps_x =  (FLOAT_TO_INT(MR.midx / MR.x_aspect)) - (MR.pw_x>>1);
    MR.ps_y = -(FLOAT_TO_INT(MR.midz / MR.y_aspect)) - (MR.pw_y>>1);
    MR.topleft_x = FLOAT_TO_INT(MR.r_x0 * ((FLOAT)(ywd->DspXRes>>1)));
    MR.topleft_y = FLOAT_TO_INT(MR.r_y0 * ((FLOAT)(ywd->DspYRes>>1)));

    /*** setze ClipRect für Linien ***/
    clip.xmin = MR.topleft_x;
    clip.ymin = MR.topleft_y;
    clip.xmax = MR.topleft_x + MR.pw_x - 1;
    clip.ymax = MR.topleft_y + MR.pw_y - 1;
    _methoda(ywd->GfxObject, RASTM_IntClipRegion, &clip);

    /*** Start-Sektor und End-Sektor ermitteln ***/
    sec_x = FLOAT_TO_INT(MR.ps_x*MR.x_aspect) / (LONG)SECTOR_SIZE;
    sec_y = FLOAT_TO_INT(MR.ps_y*MR.y_aspect) / (LONG)SECTOR_SIZE;
    end_x = FLOAT_TO_INT((MR.ps_x+MR.pw_x)*MR.x_aspect) / (LONG)SECTOR_SIZE;
    end_y = FLOAT_TO_INT((MR.ps_y+MR.pw_y)*MR.y_aspect) / (LONG)SECTOR_SIZE;

    if (sec_x < 1) sec_x = 1;
    if (sec_y < 1) sec_y = 1;
    if (end_x >= ywd->MapSizeX) end_x = ywd->MapSizeX-1;
    if (end_y >= ywd->MapSizeY) end_y = ywd->MapSizeY-1;

    /*** oki doki, also mal die Sektoren scannen... ***/
    store_sec_x = sec_x;
    num_bacts   = 0;
    for (sec_y; sec_y <= end_y; sec_y++) {
        for (sec_x = store_sec_x; sec_x <= end_x; sec_x++) {
            if (num_bacts < 512) {
    
                struct Cell *sec = &(ywd->CellArea[sec_y * ywd->MapSizeX + sec_x]);

                /*** für jede Bakterie im Sektor... ***/
                if (sec->FootPrint & MR.footprint) {
                    ls = (struct MinList *) &(sec->BactList);
                    for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {
                        struct Bacterium *b = (struct Bacterium *) nd;
                        ULONG is_clipped = FALSE;
                        
                        /*** falls Map offen, gegen Map "clippen" ***/
                        if (!(MR.req.flags & REQF_Closed)) {
                            LONG mx,my;
                            LONG x0,y0,x1,y1;
                            x0 = MR.req.req_cbox.rect.x - (ywd->DspXRes>>1);
                            x1 = x0 + MR.req.req_cbox.rect.w;
                            y0 = MR.req.req_cbox.rect.y - (ywd->DspYRes>>1);
                            y1 = y0 + MR.req.req_cbox.rect.h;
                            yw_MapGetXZ(b->pos.x,b->pos.z,&mx,&my);
                            if ((mx>x0) && (mx<x1) && (my>y0) && (my<y1)) {
                                /*** verdeckt ***/
                                is_clipped = TRUE;
                            };
                        }; 

                        if (BCLID_YPAROBO == b->BactClassID) {
                            /*** Robo-Sonderbehandlung ***/
                            if (!is_clipped) {
                                new_font(str,MAP_FONT_ROBO);
                                str = yw_RenderMapBact(ywd,str,
                                      ywd->Fonts[MAP_FONT_ROBO]->height,
                                      ywd->Fonts[MAP_FONT_ROBO]->fchars[MAP_CHAR_ROBO_HUGE].width,
                                      b, FALSE);
                                new_font(str,font_id);
                                num_bacts++;
                            };
                        } else {
                            /*** Robo-Flaks ignorieren ***/
                            if (BCLID_YPAGUN == b->BactClassID) {
                                /*** isses eine Robo-Flak? ***/
                                struct ypagun_data *yd;
                                yd = INST_DATA( ((struct nucleusdata *)b->BactObject)->o_Class, b->BactObject);
                                if (!(yd->flags & GUN_RoboGun)) {
                                    /*** es isset keine, also rendern ***/
                                    if (!is_clipped) {
                                        str = yw_RenderMapBact(ywd,str,char_width,
                                              char_height,b,do_type_icon);
                                        num_bacts++;
                                    };
                                };
                            } else {
                                if (!is_clipped) {
                                    str = yw_RenderMapBact(ywd,str,char_width,
                                          char_height,b,do_type_icon);
                                    num_bacts++;
                                };
                            };
                        };
                    };
                };
            };
        };
    };

    /*** EOS setzen und zeichnen ***/
    eos(str);
    _DrawText(&dt_args);

    /*** yo, africaan people, unite! ***/
}

