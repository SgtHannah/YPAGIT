#ifndef YPA_YPACOLOR_H
#define YPA_YPACOLOR_H
/*
**  $Source: PRG:VFM/Include/ypa/ypacolor.h,v $
**  $Revision: 38.1 $
**  $Date: 1998/01/06 14:24:57 $
**  $Locker:  $
**  $Author: floh $
**
**	Alle Farbdefinitionen.
**
**  (C) Copyright 1997 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

/*-----------------------------------------------------------------*/
struct ypa_Color {
	UBYTE r,g,b,i;		// Red, Green, Blue, 8-Bit-Index
	ULONG rgb;			// RGB verknuepft
};

#define yw_Red(ywd,id) 		(ywd->Colors[id].r)
#define yw_Green(ywd,id) 	(ywd->Colors[id].g)
#define yw_Blue(ywd,id) 	(ywd->Colors[id].b)

/*-----------------------------------------------------------------*/

#define YPACOLOR_NUM						(70)								

/*** Rassen ***/
#define YPACOLOR_OWNER_0					(0)
#define YPACOLOR_OWNER_1					(1)
#define YPACOLOR_OWNER_2					(2)
#define YPACOLOR_OWNER_3					(3)
#define YPACOLOR_OWNER_4					(4)
#define YPACOLOR_OWNER_5					(5)
#define YPACOLOR_OWNER_6					(6)
#define YPACOLOR_OWNER_7 					(7)

/*** Map ***/
#define YPACOLOR_MAP_DIRECTION				(8)
#define YPACOLOR_MAP_PRIMTARGET				(9)
#define YPACOLOR_MAP_SECTARGET				(10)
#define YPACOLOR_MAP_COMMANDER				(11)
#define YPACOLOR_MAP_DRAGBOX  				(12)
#define YPACOLOR_MAP_VIEWER					(13)		

/*** HUD ***/
#define YPACOLOR_HUD_WEAPON_0                 (20)    
#define YPACOLOR_HUD_COMPASS_COMMANDVEC_0  	  (21)
#define YPACOLOR_HUD_COMPASS_PRIMTARGET_0     (22)
#define YPACOLOR_HUD_COMPASS_LOCKTARGET_0     (23)
#define YPACOLOR_HUD_COMPASS_COMPASS_0        (24)
#define YPACOLOR_HUD_VEHICLE_0                (25)
#define YPACOLOR_HUD_VISOR_MG_0               (26)
#define YPACOLOR_HUD_VISOR_LOCKED_0           (27)
#define YPACOLOR_HUD_VISOR_AUTONOM_0          (28)

#define YPACOLOR_HUD_WEAPON_1                 (29)    
#define YPACOLOR_HUD_COMPASS_COMMANDVEC_1  	  (30)
#define YPACOLOR_HUD_COMPASS_PRIMTARGET_1     (31)
#define YPACOLOR_HUD_COMPASS_LOCKTARGET_1     (32)
#define YPACOLOR_HUD_COMPASS_COMPASS_1        (33)
#define YPACOLOR_HUD_VEHICLE_1                (34)
#define YPACOLOR_HUD_VISOR_MG_1               (35)
#define YPACOLOR_HUD_VISOR_LOCKED_1           (36)
#define YPACOLOR_HUD_VISOR_AUTONOM_1          (37)

/*** Missionbriefing ***/
#define YPACOLOR_BRIEF_LINES				(50)

/*** Textfarben ***/
#define YPACOLOR_TEXT_DEFAULT				(60)
#define YPACOLOR_TEXT_LIST					(61)
#define YPACOLOR_TEXT_LIST_SEL				(62)
#define YPACOLOR_TEXT_TOOLTIP				(63)
#define YPACOLOR_TEXT_MESSAGE				(64)
#define YPACOLOR_TEXT_HUD					(65)
#define YPACOLOR_TEXT_BRIEFING				(66)
#define YPACOLOR_TEXT_DEBRIEFING			(67)
#define YPACOLOR_TEXT_BUTTON				(68)

/*-----------------------------------------------------------------*/
#endif




