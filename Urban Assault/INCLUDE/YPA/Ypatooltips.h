#ifndef YPA_YPATOOLTIPS_H
#define YPA_YPATOOLTIPS_H
/*
**  $Source: PRG:VFM/Include/ypa/ypatooltips.h,v $
**  $Revision: 38.5 $
**  $Date: 1998/01/06 14:29:00 $
**  $Locker:  $
**  $Author: floh $
**
**  Definitionen für Tooltips
**
**  (C) Copyright 1996 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

/*-------------------------------------------------------------------
**  Alle Meldungen des Tooltip-System, geordnet nach
**  Prioritäten, höhere Nummern bedeutet höhere
**  Priorität!
*/

#define MAX_NUM_TOOLTIPS (TOOLTIP_LAST + 1)

/*** kein Tooltip anzeigen ***/
#define TOOLTIP_NONE            (0)

/*** GUI-Online-Hilfen ***/
#define TOOLTIP_GUI_ORDER       (1)
#define TOOLTIP_GUI_FIGHT       (2)
#define TOOLTIP_GUI_NEW         (3)
#define TOOLTIP_GUI_ADD         (4)
#define TOOLTIP_GUI_CONTROL     (5)
#define TOOLTIP_GUI_BUILD       (6)
#define TOOLTIP_GUI_BEAM        (7)

#define TOOLTIP_GUI_MAP         (8)
#define TOOLTIP_GUI_FINDER      (9)
#define TOOLTIP_GUI_LOG         (10)
#define TOOLTIP_GUI_ENERGY      (11)

#define TOOLTIP_GUI_AGGR_0      (12)
#define TOOLTIP_GUI_AGGR_1      (13)
#define TOOLTIP_GUI_AGGR_2      (14)
#define TOOLTIP_GUI_AGGR_3      (15)
#define TOOLTIP_GUI_AGGR_4      (16)

#define TOOLTIP_GUI_TOROBO      (17)
#define TOOLTIP_GUI_TOCMD       (18)
#define TOOLTIP_GUI_NEXTUNIT    (19)
#define TOOLTIP_GUI_NEXTCMDR    (20)
#define TOOLTIP_GUI_EXIT        (21)

#define TOOLTIP_GUI_MAP_LAND        (22)
#define TOOLTIP_GUI_MAP_OWNER       (23)
#define TOOLTIP_GUI_MAP_EXTINFO     (24)
#define TOOLTIP_GUI_MAP_NOLOCK      (25)
#define TOOLTIP_GUI_MAP_LOCKVWR     (26)
#define TOOLTIP_GUI_MAP_LOCKSEL     (27)
#define TOOLTIP_GUI_MAP_ZOOMOUT     (28)
#define TOOLTIP_GUI_MAP_ZOOMIN      (29)
#define TOOLTIP_GUI_MAP_SIZE        (30)

#define TOOLTIP_GUI_FINDER_ACTION_WAIT      (31)
#define TOOLTIP_GUI_FINDER_ACTION_FIGHT     (32)
#define TOOLTIP_GUI_FINDER_ACTION_GOTO      (33)
#define TOOLTIP_GUI_FINDER_ACTION_ESCAPE    (34)

#define TOOLTIP_GUI_ENERGY_RELOAD       (35)
#define TOOLTIP_GUI_ENERGY_SYSAKKU      (36)
#define TOOLTIP_GUI_ENERGY_VHCLAKKU     (37)
#define TOOLTIP_GUI_ENERGY_BUILDAKKU    (38)
#define TOOLTIP_GUI_ENERGY_BEAMAKKU     (39)

#define TOOLTIP_GUI_MODE        (40)
#define TOOLTIP_GUI_WEAPONS     (41)
#define TOOLTIP_GUI_VEHICLES    (42)
#define TOOLTIP_GUI_BUILDINGS   (43)

#define TOOLTIP_GUI_HUD         (44)
#define TOOLTIP_GUI_CTRL2LM     (45)    // Control-2-LastMsg

#define TOOLTIP_GUI_FINDER_NUMVHCLS     (46)
#define TOOLTIP_GUI_HELP                (47)
#define TOOLTIP_GUI_ONLINEHELP          (48)

/*** Aktionen ***/
#define TOOLTIP_ACTION_SELECT           (64)
#define TOOLTIP_ACTION_GOTO             (65)
#define TOOLTIP_ACTION_ATTACK_SEC       (66)
#define TOOLTIP_ACTION_ATTACK_VHCL      (67)
#define TOOLTIP_ACTION_NEW              (68)
#define TOOLTIP_ACTION_ADD              (69)
#define TOOLTIP_ACTION_CONTROL          (70)
#define TOOLTIP_ACTION_BUILD            (71)
#define TOOLTIP_ACTION_BEAM             (72)

/*** Weitere Tooltips, ungeordnet ***/
#define TOOLTIP_SHELL_EXITDEBRIEFING    (73)
#define TOOLTIP_SHELL_GOTOLOADSAVE      (74)
#define TOOLTIP_SHELL_SETTINGS3D        (75)
#define TOOLTIP_SHELL_SENDMESSAGE       (76)
#define TOOLTIP_SHELL_ALTJOYSTICK       (77)
#define TOOLTIP_SHELL_16BITTEXTURE      (78)
#define TOOLTIP_SHELL_DRAWPRIMITIVE     (79)
#define TOOLTIP_GUI_MOUSEMODE           (80)

/*** Fehlermeldungen ***/
#define TOOLTIP_ERROR_NOTARGET          (96)    // Mode Select
#define TOOLTIP_ERROR_NOROOM            (97)    // Mode New, Add
#define TOOLTIP_ERROR_NOENERGY          (98)    // Mode New, Add, Build, Beam
#define TOOLTIP_ERROR_NOTCONQUERED      (99)    // Build, Beam
#define TOOLTIP_ERROR_TOOFAR            (100)   // Build: zu weit weg
#define TOOLTIP_ERROR_TOOCLOSE          (101)   // Build: zu nahe
#define TOOLTIP_ERROR_JUSTBUILDING      (102)   // Build: es wird gerade gebaut
#define TOOLTIP_ERROR_VHCLSINSECTOR     (103)   // Beam: Fahrzeuge im Sektor
#define TOOLTIP_ERROR_TOOJAGGY          (104)   // Beam: Gelände zu uneben
#define TOOLTIP_ERROR_UNKNOWNAREA       (105)   // bei Karten-Operationen

/*** Die Shellmeldungen ***/
#define TOOLTIP_SHELL_PLAY              (106)
#define TOOLTIP_SHELL_TUTORIAL          (107)
#define TOOLTIP_SHELL_LOCALEOK          (108)
#define TOOLTIP_SHELL_LOCALECANCEL      (109)
#define TOOLTIP_SHELL_NEWLEVEL          (110)
#define TOOLTIP_SHELL_SEND              (111)
#define TOOLTIP_SHELL_USERRACE          (112)
#define TOOLTIP_SHELL_KYTERNESER        (113)
#define TOOLTIP_SHELL_MYKONIER          (114)
#define TOOLTIP_SHELL_TAERKASTEN        (115)
#define TOOLTIP_SHELL_READY             (116)
#define TOOLTIP_SHELL_NOTREADY          (117)
#define TOOLTIP_SHELL_LOAD              (118)
#define TOOLTIP_SHELL_SAVE              (119)
#define TOOLTIP_SHELL_OCINPUT           (120)
#define TOOLTIP_SHELL_OCVIDEO           (121)
#define TOOLTIP_SHELL_OCSOUND           (122)
#define TOOLTIP_SHELL_OCDISK            (123)
#define TOOLTIP_SHELL_OCLOCALE          (124)
#define TOOLTIP_SHELL_PL_PLAY           (125)   // Play/Pause
#define TOOLTIP_SHELL_PL_STOP           (126)   // cancel MB
#define TOOLTIP_SHELL_PL_LOAD           (127)   // Obsolete
#define TOOLTIP_SHELL_PL_FASTFORWARD    (128)   // Vorspulen
#define TOOLTIP_SHELL_PL_SETBACK        (129)   // Rücksetzen
#define TOOLTIP_SHELL_PL_GAME           (130)   // Spiel starten
#define TOOLTIP_SHELL_QUITGAME          (131)
#define TOOLTIP_SHELL_QUALIFIER         (132)
#define TOOLTIP_SHELL_ENTERINPUT        (133)
#define TOOLTIP_SHELL_PLAYLEFT          (134)
#define TOOLTIP_SHELL_PLAYRIGHT         (135)
#define TOOLTIP_SHELL_OCNET             (136)
#define TOOLTIP_SHELL_HELP              (137)
#define TOOLTIP_SHELL_PL_TOTITLE        (138)
#define TOOLTIP_SHELL_PL_TOPLAY         (139)
#define TOOLTIP_SHELL_NETCANCEL         (140)
#define TOOLTIP_SHELL_NETHELP           (141)
#define TOOLTIP_SHELL_OKPROVIDER        (142)
#define TOOLTIP_SHELL_OKPLAYER          (143)
#define TOOLTIP_SHELL_OKLEVEL           (144)
#define TOOLTIP_SHELL_OKSESSION         (145)
#define TOOLTIP_SHELL_STARTLEVEL        (146)
#define TOOLTIP_SHELL_BACKTOPROVIDER    (147)
#define TOOLTIP_SHELL_BACKTOPLAYER      (148)
#define TOOLTIP_SHELL_BACKTOSESSION     (149)


#define TOOLTIP_SHELL_DB_STOP           (151) // <OBSOLETE>
#define TOOLTIP_SHELL_DB_REWIND         (152) // Debriefing zurückspulen
#define TOOLTIP_SHELL_DB_TIME_LOC       (153) // <OBSOLETE>
#define TOOLTIP_SHELL_DB_TIME_GLOB      (154) // <OBSOLETE>
#define TOOLTIP_SHELL_DB_KILLS_LOC      (155) // <OBSOLETE>
#define TOOLTIP_SHELL_DB_KILLS_GLOB     (156) // <OBSOLETE>
#define TOOLTIP_SHELL_DB_LOSS_LOC       (157) // <OBSOLETE>
#define TOOLTIP_SHELL_DB_LOSS_GLOB      (158) // <OBSOLETE>
#define TOOLTIP_SHELL_DB_SECS_LOC       (159) // <OBSOLETE>
#define TOOLTIP_SHELL_DB_SECS_GLOB      (160) // <OBSOLETE>
#define TOOLTIP_SHELL_DB_SCORE_LOC      (161) // <OBSOLETE>
#define TOOLTIP_SHELL_DB_SCORE_GLOB     (162) // <OBSOLETE>
#define TOOLTIP_SHELL_DB_POWER_LOC      (163) // <OBSOLETE>
#define TOOLTIP_SHELL_DB_POWER_GLOB     (164) // <OBSOLETE>
#define TOOLTIP_SHELL_DB_UPGRADE_LOC    (165) // <OBSOLETE>
#define TOOLTIP_SHELL_DB_UPGRADE_GLOB   (166) // <OBSOLETE>
#define TOOLTIP_SHELL_DB_RACE           (167) // <OBSOLETE>

/*** weiter Shell ***/
#define TOOLTIP_SHELL_DELETE            (168)
#define TOOLTIP_SHELL_NEWGAME           (169)
#define TOOLTIP_SHELL_OKLOAD            (170)
#define TOOLTIP_SHELL_OKSAVE            (171)
#define TOOLTIP_SHELL_OKDELETE          (172)
#define TOOLTIP_SHELL_OKCREATE          (173)
#define TOOLTIP_SHELL_DISKCANCEL        (174)
#define TOOLTIP_SHELL_CANCELLOAD        (175)
#define TOOLTIP_SHELL_CANCELSAVE        (176)
#define TOOLTIP_SHELL_CANCELDELETE      (177)
#define TOOLTIP_SHELL_CANCELCREATE      (178)
#define TOOLTIP_SHELL_SETTINGSCANCEL    (179)
#define TOOLTIP_SHELL_SETTINGSOK        (180)
#define TOOLTIP_SHELL_RESOLUTION        (181)
#define TOOLTIP_SHELL_SOUNDSWITCH       (182)
#define TOOLTIP_SHELL_SOUNDLR           (183)
#define TOOLTIP_SHELL_FARVIEW           (184)
#define TOOLTIP_SHELL_HEAVEN            (185)
#define TOOLTIP_SHELL_FILTERING         (186)
#define TOOLTIP_SHELL_CDSOUND           (187)
#define TOOLTIP_SHELL_SOFTMOUSE         (188)
#define TOOLTIP_SHELL_ENEMYINDICATOR    (189)
#define TOOLTIP_SHELL_FXVOLUME          (190)
#define TOOLTIP_SHELL_CDVOLUME          (191)
#define TOOLTIP_SHELL_FXSLIDER          (192)
#define TOOLTIP_SHELL_INPUTOK           (193)
#define TOOLTIP_SHELL_INPUTCANCEL       (194)
#define TOOLTIP_SHELL_INPUTRESET        (195)
#define TOOLTIP_SHELL_JOYSTICK          (196)
#define TOOLTIP_SHELL_FORCEFEEDBACK     (197)
#define TOOLTIP_SHELL_SWITCHOFF         (198)


// ***ANPASSEN***
#define TOOLTIP_LAST    (201)

/*-----------------------------------------------------------------*/
#endif

