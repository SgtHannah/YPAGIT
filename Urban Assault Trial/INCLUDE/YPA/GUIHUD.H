#ifndef YPA_GUIHUD_H
#define YPA_GUIHUD_H
/*
**  $Source: PRG:VFM/Include/ypa/guihud.h,v $
**  $Revision: 38.3 $
**  $Date: 1998/01/06 14:22:38 $
**  $Locker:  $
**  $Author: floh $
**
**  Definitionen für Head-Up-Display in den Vehikeln.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#define NUM_HUDVEC              (14)

#define HUDVEC_BAR              (0)     // innerer Balken
#define HUDVEC_COMPASS          (1)     // Kompass-Outline
#define HUDVEC_FRAME            (2)     // allgemeiner Rahmen
#define HUDVEC_ARROW            (3)     // Kompass-Pfeil(e)
#define HUDVEC_MG_VISOR         (4)     // MG-Visier

#define HUDVEC_GRENADE_VISOR_1  (5)     // Granaten-Visier #1
#define HUDVEC_ROCKET_VISOR_1   (6)     // ungelenkte Raketen Visier #1
#define HUDVEC_MISSILE_VISOR_1  (7)     // gelenkte Raketen Visier #1
#define HUDVEC_BOMB_VISOR_1     (8)     // Bomben, Visier #1

#define HUDVEC_GRENADE_VISOR_2  (9)     // Granaten-Visier, #2
#define HUDVEC_ROCKET_VISOR_2   (10)    // ungelenkte Raketen Visier, #2
#define HUDVEC_MISSILE_VISOR_2  (11)    // gelenkte Raketen Visier, #2
#define HUDVEC_BOMB_VISOR_2     (12)    // Bomben, Visier #2

#define HUDVEC_TRIANGLE			(13)

/*** Filenamen ***/
#define HUDVEC_BAR_NAME                 "wireless/bar.sklt"
#define HUDVEC_COMPASS_NAME             "wireless/compass.sklt"
#define HUDVEC_FRAME_NAME               "wireless/frame.sklt"
#define HUDVEC_ARROW_NAME               "wireless/arrow.sklt"

#define HUDVEC_MG_VISOR_NAME            "wireless/mgvis.sklt"
#define HUDVEC_GRENADE_VISOR_1_NAME     "wireless/grenvis1.sklt"
#define HUDVEC_ROCKET_VISOR_1_NAME      "wireless/rockvis1.sklt"
#define HUDVEC_MISSILE_VISOR_1_NAME     "wireless/missvis1.sklt"
#define HUDVEC_BOMB_VISOR_1_NAME        "wireless/bombvis1.sklt"

#define HUDVEC_GRENADE_VISOR_2_NAME     "wireless/grenvis2.sklt"
#define HUDVEC_ROCKET_VISOR_2_NAME      "wireless/rockvis2.sklt"
#define HUDVEC_MISSILE_VISOR_2_NAME     "wireless/missvis2.sklt"
#define HUDVEC_BOMB_VISOR_2_NAME        "wireless/bombvis2.sklt"

#define HUDVEC_TRIANGLE_NAME			"wireless/triangle.sklt"

struct YPAHud {
    /*** Layout-Stuff ***/
    UWORD RenderHUD;                // TRUE: HUD rendern

    /*** Skeletons als 2D-Vektor-Outlines ***/
    Object *HudVecObj[NUM_HUDVEC];
    struct Skeleton *HudVecSklt[NUM_HUDVEC];
    ULONG is_hicolor;

    /*** Timer fuer HUD-Effekte ***/
    ULONG change_vhcl_timer;            // auf TimeStamp, wenn Vehikel gewechselt
    ULONG change_target_timer;          // auf TimeStamp, wenn Visier neu gelockt
    struct Bacterium *prev_target;      // des Visiers
    LONG breath_time;                   // "Atem-Frequenz" des HUD
	FLOAT breath_pos;					// aktuelle "Atem-Position", (0..breath_time)

	/*** Vehicle Information Display Informationen ***/
	FLOAT vid_w, vid_h;					// relative Hoehe und Breite
	FLOAT vid_text_h;					// relative Hoehe einer Text-Zeile
	LONG vid_prefix_w;
	LONG vid_postfix_w;
	LONG vid_numtiles;

	/*** Copper-Effekt-Temps ***/
	FLOAT r0,g0,b0;
	FLOAT r1,g1,b1;
};

#define HUDVID_WEAPONBLINK		(1<<0)
#define HUDVID_ENERGYBLINK		(1<<1)
#define HUDVID_SHIELDBLINK		(1<<2)
#define HUDVID_NAMEBLINK		(1<<3)
#define HUDVID_SCALEUP			(1<<4)

/*** OBSOLETE *** OBSOLETE *** OBSOLETE ***/
#define HUDCOL_WEAPON                     ((112<<16)|(200<<8)|(128))
#define HUDCOL_RELOADBAR_READY            ((142<<16)|(175<<8)|(182))
#define HUDCOL_RELOADBAR_LOAD             ((125<<16)|(196<<8)|(173))
#define HUDCOL_RELOADBAR_REST             ((125<<16)|(125<<8)|(125))
#define HUDCOL_ENERGYBAR_NOCHANGE         ((136<<16)|(136<<8)|(136))
#define HUDCOL_ENERGYBAR_LOAD_REST        ((125<<16)|(125<<8)|(125))
#define HUDCOL_FORCEBAR_NOFORCE           ((125<<16)|(125<<8)|(125))
#define HUDCOL_FORCEBAR_POSFORCE          ((142<<16)|(175<<8)|(182))
#define HUDCOL_FORCEBAR_NEGFORCE          ((126<<16)|(155<<8)|(161))
#define HUDCOL_FORCEBAR_REST              ((125<<16)|(125<<8)|(125))
#define HUDCOL_SPEEDBAR_SPEED             ((142<<16)|(175<<8)|(182))
#define HUDCOL_SPEEDBAR_REST              ((125<<16)|(125<<8)|(125))
#define HUDCOL_LIFEBAR_LIFE               ((193<<16)|(226<<8)|(233))
#define HUDCOL_LIFEBAR_REST               ((160<<16)|(160<<8)|(160))
#define HUDCOL_COMPASS_COMMANDVEC         ((73 <<16)|(131<<8)|(178))
#define HUDCOL_COMPASS_PRIMTARGET         ((123<<16)|(225<<8)|(255))
#define HUDCOL_COMPASS_LOCKTARGET         ((255<<16)|(0  <<8)|(0))
#define HUDCOL_COMPASS_COMPASS            ((142<<16)|(175<<8)|(182))
#define HUDCOL_VEHICLE                    ((142<<16)|(175<<8)|(182))
#define HUDCOL_VISOR_MG                   ((142<<16)|(175<<8)|(182))
#define HUDCOL_VISOR_LOCKED               ((255<<16)|(0  <<8)|(0))
#define HUDCOL_VISOR_AUTONOM              ((142<<16)|(175<<8)|(182))
#define HUDCOL_OWNER_0                    ((100<<16)|(100<<8)|(100))
#define HUDCOL_OWNER_1                    ((0  <<16)|(123<<8)|(217))
#define HUDCOL_OWNER_2                    ((0  <<16)|(172<<8)|(0))
#define HUDCOL_OWNER_3                    ((255<<16)|(255<<8)|(255))
#define HUDCOL_OWNER_4                    ((255<<16)|(191<<8)|(31))
#define HUDCOL_OWNER_5                    ((129<<16)|(129<<8)|(129))
#define HUDCOL_OWNER_6                    ((237<<16)|(0  <<8)|(0))
#define HUDCOL_OWNER_7                    ((0  <<16)|(0  <<8)|(0))
/*-----------------------------------------------------------------*/
#endif




