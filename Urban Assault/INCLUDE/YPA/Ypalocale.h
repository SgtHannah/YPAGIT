#ifndef YPA_YPALOCALE_H
#define YPA_YPALOCALE_H
/*
**  $Source: PRG:VFM/Include/ypa/ypalocale.h,v $
**  $Revision: 38.14 $
**  $Date: 1998/01/14 17:08:24 $
**  $Locker:  $
**  $Author: floh $
**
**  Definitionen f�r das YPA-Localizing-System.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

/*-------------------------------------------------------------------
**  Background:
**  -----------
**  Ein String ist definiert durch eine numerische Konstante,
**  mit dieser Konstante und dem Macro ypa_GetStr() kann
**  der Pointer auf den String angefordert werden:
**
**      UBYTE *ypa_GetStr(APTR locale, ULONG id, UBYTE *default);
**
**  locale  - Locale-Handle, wird ermittelt per _get(YWA_LocaleHandle)
**  id      - die Locale-ID des Strings (z.B. STR_YES)
**  default - Default-String, falls keine Locale-Definition vorhanden
**
**  Resultat ist ein String-Pointer.
**
**  Internals:
**  ~~~~~~~~~~
**  Eigentlich wollte ich das Locale-System �ber ein gigantisches
**  String-Pointer-Array realisieren, wobei jeder String in
**  einem MemBlock liegt -> Zuviel Overhead und Speicher-
**  Fragmentierung. Vielmehr arbeite ich jetzt mit einem
**  einzigen 16KB Speicher-Block, in dem alle String hintereinander
**  reingeschrieben werden (Implementierungs-Detail spare ich
**  mit hier). Dann arbeite ich mit einem Pointer-Array, die
**  Pointer zeigen in den String-Puffer. Nachteil: kein
**  "Random-Read/Write-Zugriff", aber das ist vernachl�ssigbar,
**  weil sowieso ein komplettes Locale-String-Set vorhanden
**  sein muss.
*/

static UBYTE *ypa_GetStr(UBYTE **loc, ULONG id, UBYTE *def)
{
    UBYTE *loc_str = loc[id];
    if ((loc_str == NULL) || (strcmp(loc_str,"<>")==0)) loc_str=def;
    return(loc_str);
}

/*** Konstanten-Definition ***/
#define LOCALE_MAX_STRINGS   (STR_LAST_ITEM)      // ANPASSEN!!!
#define LOCALE_SIZEOF_STRBUF (128*1024)

/*** allgemeine Strings ***/
#define STR_YES             (0)
#define STR_NO              (1)
#define STR_OK              (2)
#define STR_CANCEL          (3)
#define STR_LOAD            (4)
#define STR_SAVE            (5)
#define STR_WELCOME         (6)
#define STR_CANCELMISSION   (7)
#define STR_RESTART         (8)
#define STR_RESUME          (9)
#define STR_DRONEBROKEN     (10)    // Drone im Arsch
#define STR_ROBOBROKEN      (11)    // Robo im Arsch
#define STR_CONTINUED       (12)
#define STR_RESET           (13)
#define STR_PAUSED          (14)    // Game Paused...
#define STR_FONTDEFINE          (15)    // "fontface,height,weight,charset"
#define STR_FONTDEFINE_LRES     (16)    // dasselbe fuer unter Lowres-GUI-Set
#define STR_USESYSTEMTEXTINPUT  (17)    // "TRUE", oder "FALSE"
#define STR_SUPERBOMB_NAME      (18)    // Name der Superbombe
#define STR_SUPERWAVE_NAME      (19)    // Name der Superwave
#define STR_HELP                (20)    // allgemeiner Helptext fuer alle Button

/*** HUD Texte ***/
#define STR_HUD_WPNDAMAGE       (32)
#define STR_HUD_RELOAD          (33)
#define STR_HUD_RLDOK           (34)
#define STR_HUD_HITPOINTS       (35)
#define STR_HUD_ARMOR           (36)

/*** Window-Titel ***/
#define STR_WIN_MAP         (50)
#define STR_WIN_FINDER      (51)
#define STR_WIN_LOGWIN      (52)
#define STR_WIN_ABORT       (53)    // == Game Paused!
#define STR_WIN_ENERGY      (54)
#define STR_WIN_SOUND       (55)
#define STR_WIN_VIDEO       (56)
#define STR_WIN_INPUT       (57)
#define STR_WIN_DISK        (58)
#define STR_WIN_LOCALE      (59)
#define STR_WIN_ABOUT       (60)    // neu af
#define STR_WIN_NETWORK     (61)    // neu af

#define STR_TITLE_PLAY      (80)
#define STR_TITLE_NETWORK   (81)
#define STR_TITLE_TUTORIAL  (82)
#define STR_TITLE_INPUT     (83)
#define STR_TITLE_SETTINGS  (84)
#define STR_TITLE_DISK      (85)
#define STR_TITLE_LOCALE    (86)
#define STR_TITLE_HELP      (87)
#define STR_TITLE_QUIT      (88)

/*** Confirm-Req-Inhalt ***/
#define STR_CONFIRM_EXIT        (120)
#define STR_CONFIRM_SAVE        (121)
#define STR_CONFIRM_LOAD        (122)
#define STR_CONFIRM_RESTART     (123)

/*** Missionbriefing/Debriefing-Texte ***/
#define STR_MB_YOUAREHERE               (150)
#define STR_MB_PRIMARY                  (151)
#define STR_MB_UPGRADE                  (152)
#define STR_MB_ENEMYROBO                (153)
#define STR_MB_ENEMYFORCE               (154)
#define STR_MB_USERFORCE                (155)
#define STR_MB_GATES                    (156)
#define STR_MB_KEYSECTOR                (157)
#define STR_MB_TECHUPGRADE              (158)
#define STR_MB_BEAMGATE                 (159)
#define STR_MB_POWERSTATION             (160)           
#define STR_MB_TEXT                     (161)

#define STR_DB_DEBRIEFING       (170)
#define STR_DB_TIME             (171)
#define STR_DB_KILLS            (172)
#define STR_DB_LOSSES           (173)
#define STR_DB_SECTORS          (174)
#define STR_DB_SCORE            (175)
#define STR_DB_POWERSTATION     (176)
#define STR_DB_TECHUPGRADES     (177)

#define STR_LMSG_NOP                  (200)
#define STR_LMSG_DONE                 (201)
#define STR_LMSG_CONQUERED            (202)
#define STR_LMSG_FIGHTSECTOR          (203)
#define STR_LMSG_REMSECTOR            (204)
#define STR_LMSG_ENEMYDESTROYED       (205)
#define STR_LMSG_FOUNDROBO            (206)
#define STR_LMSG_FOUNDENEMY           (207)
#define STR_LMSG_LASTDIED             (208)
#define STR_LMSG_ESCAPE               (209)
#define STR_LMSG_RELAXED              (210)
#define STR_LMSG_ROBODEAD             (211)
#define STR_LMSG_USERROBODEAD         (212)
#define STR_LMSG_USERROBODANGER       (213)
#define STR_LMSG_CREATED              (214)
#define STR_LMSG_ORDERMOVE            (215)
#define STR_LMSG_ORDERATTACK          (216)
#define STR_LMSG_CONTROL              (217)
#define STR_LMSG_REQUESTSUPPORT       (218)
#define STR_LMSG_ENEMYESCAPES         (219)
#define STR_LMSG_ENEMYGROUND          (220)
#define STR_LMSG_WUNDERSTEIN          (221)
#define STR_LMSG_ENEMYSECTOR          (222)
#define STR_LMSG_GATEOPENED           (223)
#define STR_LMSG_GATECLOSED           (224)
#define STR_LMSG_TECH_WEAPON          (225)
#define STR_LMSG_TECH_ARMOR           (226)
#define STR_LMSG_TECH_VEHICLE         (227)
#define STR_LMSG_TECH_BUILDING        (228)
#define STR_LMSG_TECH_LOST            (229)
#define STR_LMSG_POWER_DESTROYED      (230)
#define STR_LMSG_FLAK_DESTROYED       (231)
#define STR_LMSG_RADAR_DESTROYED      (232)
#define STR_LMSG_POWER_ATTACK         (233)
#define STR_LMSG_FLAK_ATTACK          (234)
#define STR_LMSG_RADAR_ATTACK         (235)
#define STR_LMSG_POWER_CREATED        (236)
#define STR_LMSG_FLAK_CREATED         (237)
#define STR_LMSG_RADAR_CREATED        (238)
#define STR_LMSG_HOST_WELCOMEBACK     (239)
#define STR_LMSG_HOST_ENERGY_CRITICAL (240)
#define STR_LMSG_HOST_ONLINE          (241)
#define STR_LMSG_HOST_SHUTDOWN        (242)
#define STR_LMSG_ANALYSIS_AVAILABLE   (243)
#define STR_LMSG_ANALYSIS_WORKING     (244)
#define STR_LMSG_POWER_CAPTURED       (245)
#define STR_LMSG_WASKILLEDBY          (246) // Es gibt 4 Arten, eine Todesmeldung auszugeben. 1. wenn
#define STR_LMSG_HASDIED              (247) // irgendjemand irgendjemand gekillt hat (name WASKILLEDBY name)
#define STR_LMSG_YOUWIN               (248) // 2. wenn jemand aus Energymangel gestorben ist (name HASDIED),
#define STR_LMSG_YOUKILLED            (249) // 3. wenn ich den letzten gekilled habe (YOUWIN) oder 4. wenn
                                            // ich einen davor gekilled habe (YOUKILLED name).   
#define STR_LMSG_SUPERBOMB_ACTIVATED    (250)
#define STR_LMSG_SUPERBOMB_TRIGGERED    (251)
#define STR_LMSG_SUPERBOMB_FROZEN       (252)
#define STR_LMSG_SUPERBOMB_DEACTIVATED  (253)
#define STR_LMSG_SUPERWAVE_ACTIVATED    (254)
#define STR_LMSG_SUPERWAVE_TRIGGERED    (255)
#define STR_LMSG_SUPERWAVE_FROZEN       (256)
#define STR_LMSG_SUPERWAVE_DEACTIVATED  (257)
#define STR_LMSG_BEAMGATE_FULL          (258)
#define STR_LMSG_GAME_SAVED             (259)

#define STR_LMSG_POWER_LOST             (260)
#define STR_LMSG_FLAK_LOST              (261)
#define STR_LMSG_RADAR_LOST             (262)

/*** Shellspezifische Texte, Fenstertitel siehe oben ***/

/*** Input Requester Gadgets ***/
#define STR_IGADGET_CTRL            (300)
#define STR_IGADGET_RSHIFT          (301)
#define STR_IGADGET_ALT             (302)
#define STR_IGADGET_QUALMODE        (303)
#define STR_IGADGET_NOQUALMODE      (304)
#define STR_IGADGET_JOYSTICK        (305)
#define STR_IGADGET_FORCEFEEDBACK   (306)
#define STR_IGADGET_SWITCHOFF       (307)
#define STR_IGADGET_TOBEDEFINED     (308)   // Taste erwartet-->Menueintrag
#define STR_IGADGET_HEADLINE        (309)
#define STR_IGADGET_HEADLINE2       (310)
#define STR_IGADGET_HEADLINE3       (311)
#define STR_IGADGET_HEADLINE4       (312)

/*** Audio Requester Gadgets ***/
#define STR_SGADGET_USESOUND        (320)
#define STR_SGADGET_VOLUME          (321)
#define STR_SGADGET_CHANNELS        (322)
#define STR_SGADGET_INVERTLR        (323)
#define STR_SGADGET_CDVOLUME        (324)
#define STR_SGADGET_ENEMY           (325)
#define STR_SGADGET_USECDSOUND      (326)
#define STR_SGADGET_HEADLINE        (327)
#define STR_SGADGET_HEADLINE2       (328)
#define STR_SGADGET_HEADLINE3       (329)
#define STR_SGADGET_HEADLINE4       (330)

/*** Display Requester Settings ***/
#define STR_VGADGET_RESSHELL        (340)
#define STR_VGADGET_WARNING1        (341)
#define STR_VGADGET_WARNING2        (342)
#define STR_VGADGET_DESTFX          (343)
#define STR_VGADGET_FARVIEW         (344)
#define STR_VGADGET_HEAVEN          (345)
#define STR_VGADGET_PALETTEFX       (346)
#define STR_VGADGET_TEXTMENU        (347)
#define STR_VGADGET_OPENMENU        (348)
#define STR_VGADGET_RESGAME         (349)
#define STR_VGADGET_SOFTMOUSE       (350)
#define STR_VGADGET_FILTERING       (351)
#define STR_VGADGET_3DDEVICE        (352)

/*** User Settings Gadgets ***/
#define STR_DGADGET_LOAD            (360)
#define STR_DGADGET_SAVE            (361)
#define STR_DGADGET_DELETE          (362)
#define STR_DGADGET_NEWGAME         (363)
#define STR_DGADGET_COPY            (364)
#define STR_DGADGET_ENTERNAME       (365)
#define STR_DGADGET_NEWUSER         (366)
#define STR_DGADGET_HEADLINE        (367)
#define STR_DGADGET_HEADLINE2       (368)
#define STR_DGADGET_HEADLINE3       (369)
#define STR_DGADGET_HEADLINE4       (370)

/*** About Settings Gadgets OBSOLET ***/
#define STR_AGADGET_DESIGNER        (380)
#define STR_AGADGET_PROG1           (381)
#define STR_AGADGET_PROG2           (382)
#define STR_AGADGET_LEVEL           (383)
#define STR_AGADGET_SOUND           (384)
#define STR_AGADGET_FUN             (385)

/*** Locale ***/
#define STR_LGADGET_HEADLINE        (395)
#define STR_LGADGET_HEADLINE2       (396)
#define STR_LGADGET_HEADLINE3       (397)
#define STR_LGADGET_HEADLINE4       (398)

/*** Network Settings Gadgets ***/
#define STR_NGADGET_OK              (400)
#define STR_NGADGET_BACK            (401)
#define STR_NGADGET_NEW             (402)
#define STR_NGADGET_CANCEL          (403)
#define STR_NGADGET_START           (404)
#define STR_NGADGET_SEND            (405)
#define STR_NGADGET_JOIN            (406)
#define STR_NGADGET_NETSTART        (407)
#define STR_NGADGET_NETWAIT         (408)
#define STR_NGADGET_NETREADY        (409)
#define STR_NGADGET_HLPROVIDER      (410)
#define STR_NGADGET_HLSESSIONS      (411)
#define STR_NGADGET_HLLEVEL         (412)
#define STR_NGADGET_HLPLAYER        (413)
#define STR_NGADGET_HLSTART         (414)
#define STR_NGADGET_HLWAIT          (415)
#define STR_NGADGET_LVPROVIDER      (416)
#define STR_NGADGET_LVSESSIONS      (417)
#define STR_NGADGET_LVLEVEL         (418)
#define STR_NGADGET_LVPLAYER        (419)
#define STR_NGADGET_LVMESSAGE       (420)
#define STR_NGADGET_SEARCH          (421)
#define STR_NGADGET_ENTERMESSAGE    (422)
#define STR_NGADGET_ENTERNAME       (423)
#define STR_NGADGET_SELRACE         (424)
#define STR_NGADGET_HLPROVIDER2     (425)
#define STR_NGADGET_HLPROVIDER3     (426)
#define STR_NGADGET_HLPROVIDER4     (427)
#define STR_NGADGET_HLSESSIONS2     (428)
#define STR_NGADGET_HLSESSIONS3     (429)
#define STR_NGADGET_HLSESSIONS4     (430)
#define STR_NGADGET_HLLEVEL2        (431)
#define STR_NGADGET_HLLEVEL3        (432)
#define STR_NGADGET_HLLEVEL4        (433)
#define STR_NGADGET_HLPLAYER2       (434)
#define STR_NGADGET_HLPLAYER3       (435)
#define STR_NGADGET_HLPLAYER4       (436)
#define STR_NGADGET_HLSTART2        (437)
#define STR_NGADGET_HLSTART3        (438)
#define STR_NGADGET_HLSTART4        (439)
#define STR_NGADGET_HLWAIT2         (440)
#define STR_NGADGET_HLWAIT3         (441)
#define STR_NGADGET_HLWAIT4         (442)

/*** Input_Menu_Entries ***/
#define STR_IMENU_DRVDIR            (500)
#define STR_IMENU_DRVSPEED          (501)
#define STR_IMENU_FLYHEIGHT         (502)
#define STR_IMENU_FLYSPEED          (503)
#define STR_IMENU_FLYDIR            (504)
#define STR_IMENU_STOP              (505)
#define STR_IMENU_FIRE              (506)
#define STR_IMENU_FIREVIEW          (507)
#define STR_IMENU_FIREGUN           (508)
#define STR_IMENU_ROTVERT           (509)
#define STR_IMENU_ROTHORIZ          (510)
#define STR_IMENU_GUNHEIGHT         (511)
#define STR_IMENU_VISIER            (512)
#define STR_IMENU_ORDER             (513)
#define STR_IMENU_FIGHT             (514)
#define STR_IMENU_NEW               (515)
#define STR_IMENU_ADD               (516)
#define STR_IMENU_CONTROL           (517)
#define STR_IMENU_BUILD             (518)
#define STR_IMENU_PANIC             (519)
#define STR_IMENU_AUTOPILOT         (520)
#define STR_IMENU_MAP               (521)
#define STR_IMENU_FINDER            (522)
#define STR_IMENU_MAPLANDSCAPE      (523)
#define STR_IMENU_MAPOWNER          (524)
#define STR_IMENU_MAPHEIGHT         (525)
#define STR_IMENU_NOMAPLOCK         (526)
#define STR_IMENU_LOCKVIEWER        (527)
#define STR_IMENU_LOCKSQUAD         (528)
#define STR_IMENU_ZOOMIN            (529)
#define STR_IMENU_ZOOMOUT           (530)
#define STR_IMENU_MAPMINI           (531)
#define STR_IMENU_NEXTCOM           (532)
#define STR_IMENU_TOROBO            (533)
#define STR_IMENU_NEXTMAN           (534)
#define STR_IMENU_TOCOMMANDER       (535)
#define STR_IMENU_QUIT              (536)
#define STR_IMENU_CYCLE             (537)
#define STR_IMENU_LOGWIN            (538)
#define STR_IMENU_NEXTITEM          (539)
#define STR_IMENU_PREVITEM          (540)
#define STR_IMENU_HUD               (541)
#define STR_IMENU_ENERGY            (542)
#define STR_IMENU_LASTMSG           (543)
#define STR_IMENU_PAUSE             (544)
#define STR_IMENU_TOUSER            (545)
#define STR_IMENU_TOKYTERNESER      (546)
#define STR_IMENU_TOMYKONIER        (547)
#define STR_IMENU_TOTAERKASTEN      (548)
#define STR_IMENU_TOPLAYER5         (549)
#define STR_IMENU_TOPLAYER6         (550)
#define STR_IMENU_TOPLAYER7         (551)
#define STR_IMENU_TOALL             (552)
#define STR_IMENU_AGGR1             (553)
#define STR_IMENU_AGGR2             (554)
#define STR_IMENU_AGGR3             (555)
#define STR_IMENU_AGGR4             (556)
#define STR_IMENU_AGGR5             (557)
#define STR_IMENU_WAYPOINT          (558)
#define STR_IMENU_HELP              (559)
#define STR_IMENU_LASTOCCUPIED      (560)
#define STR_IMENU_MAKECOMMANDER     (561)

/*** allgemeine Beschreiber ***/
#define STR_NGADGET_LEVELTEXT       (600)
#define STR_SHELL_RIGHT     (601)
#define STR_SHELL_UP        (602)
#define STR_SHELL_DOWN      (603)
#define STR_SHELL_MORE      (604)
#define STR_SHELL_LESS      (605)
#define STR_SHELL_NOTSUPP   (606)
#define STR_SHELL_PLAYER    (607)   // OBSOLETE

/*** Ersatz fuer Balken ***/
#define STR_SHELL_SETBACK   (640)
#define STR_SHELL_STEPFORWARD   (641)
#define STR_SHELL_LOADGAME  (642)
#define STR_SHELL_STARTGAME (643)
#define STR_SHELL_GOBACK    (644)

/*** Shell: Netzwerk Misc ***/
#define STR_NET_MESSAGETO           (650)
#define STR_NET_WAITING             (651)
#define STR_NET_ALL                 (652)
#define STR_NET_REMOVEPLAYER        (653)
#define STR_NET_NOCONNECTION        (654)
#define STR_NET_YOUWIN              (655)

/*** Game State Analyzer Meldungen [700.. ] ***/
#define STR_GSTATE_ADVICE1      (700)
#define STR_GSTATE_ADVICE2      (701)
#define STR_GSTATE_ADVICE3      (702)
#define STR_GSTATE_ADVICE4      (703)
#define STR_GSTATE_ADVICE5      (704)
#define STR_GSTATE_ADVICE6      (705)
#define STR_GSTATE_ADVICE7      (706)
#define STR_GSTATE_ADVICE8      (707)
#define STR_GSTATE_ADVICE9      (708)
#define STR_GSTATE_ADVICE10     (709)
#define STR_GSTATE_ADVICE11     (710)
#define STR_GSTATE_ADVICE12     (711)
#define STR_GSTATE_ADVICE13     (712)
#define STR_GSTATE_ADVICE14     (713)
#define STR_GSTATE_ADVICE15     (714)
#define STR_GSTATE_ADVICE16     (715)
#define STR_GSTATE_ADVICE17     (716)

/*** Online Hilfe URLs ***/
#define STR_HELP_ROOT               (750)
#define STR_HELP_LEVELSELECT        (751)
#define STR_HELP_TUTSELECT          (752)
#define STR_HELP_MULTI_SELPROV      (753)
#define STR_HELP_MULTI_ENTERNAME    (754)
#define STR_HELP_MULTI_SELSESSION   (755)
#define STR_HELP_MULTI_SELLEVEL     (756)
#define STR_HELP_MULTI_STARTSCREEN  (757)
#define STR_HELP_SAVEGAME           (758)
#define STR_HELP_INPUTCONFIG        (759)
#define STR_HELP_SETTINGS           (760)
#define STR_HELP_LANGUAGE           (761)
#define STR_HELP_BRIEFING           (762)
#define STR_HELP_DEBRIEFING         (763)
#define STR_HELP_INGAMEMAP          (764)
#define STR_HELP_INGAMEFINDER       (765)
#define STR_HELP_INGAMEMENU         (766)
#define STR_HELP_INGAMEGENERAL      (767)

/*** TOOLTIP_SYSTEM ***/
#define STR_TIP_NONE        (800)

/*** GUI-Online-Hilfen ***/
#define STR_TIP_GUI_ORDER       (STR_TIP_NONE+TOOLTIP_GUI_ORDER)
#define STR_TIP_GUI_FIGHT       (STR_TIP_NONE+TOOLTIP_GUI_FIGHT)
#define STR_TIP_GUI_NEW         (STR_TIP_NONE+TOOLTIP_GUI_NEW)
#define STR_TIP_GUI_ADD         (STR_TIP_NONE+TOOLTIP_GUI_ADD)
#define STR_TIP_GUI_CONTROL     (STR_TIP_NONE+TOOLTIP_GUI_CONTROL)
#define STR_TIP_GUI_BUILD       (STR_TIP_NONE+TOOLTIP_GUI_BUILD)
#define STR_TIP_GUI_BEAM        (STR_TIP_NONE+TOOLTIP_GUI_BEAM)

#define STR_TIP_GUI_MAP         (STR_TIP_NONE+TOOLTIP_GUI_MAP)
#define STR_TIP_GUI_FINDER      (STR_TIP_NONE+TOOLTIP_GUI_FINDER)
#define STR_TIP_GUI_LOG         (STR_TIP_NONE+TOOLTIP_GUI_LOG)
#define STR_TIP_GUI_ENERGY      (STR_TIP_NONE+TOOLTIP_GUI_ENERGY)

#define STR_TIP_GUI_AGGR_0      (STR_TIP_NONE+TOOLTIP_GUI_AGGR_0)
#define STR_TIP_GUI_AGGR_1      (STR_TIP_NONE+TOOLTIP_GUI_AGGR_1)
#define STR_TIP_GUI_AGGR_2      (STR_TIP_NONE+TOOLTIP_GUI_AGGR_2)
#define STR_TIP_GUI_AGGR_3      (STR_TIP_NONE+TOOLTIP_GUI_AGGR_3)
#define STR_TIP_GUI_AGGR_4      (STR_TIP_NONE+TOOLTIP_GUI_AGGR_4)

#define STR_TIP_GUI_TOROBO      (STR_TIP_NONE+TOOLTIP_GUI_TOROBO)
#define STR_TIP_GUI_TOCMD       (STR_TIP_NONE+TOOLTIP_GUI_TOCMD)
#define STR_TIP_GUI_NEXTUNIT    (STR_TIP_NONE+TOOLTIP_GUI_NEXTUNIT)
#define STR_TIP_GUI_NEXTCMDR    (STR_TIP_NONE+TOOLTIP_GUI_NEXTCMDR)
#define STR_TIP_GUI_EXIT        (STR_TIP_NONE+TOOLTIP_GUI_EXIT)

#define STR_TIP_GUI_MAP_LAND        (STR_TIP_NONE+TOOLTIP_GUI_MAP_LAND)
#define STR_TIP_GUI_MAP_OWNER       (STR_TIP_NONE+TOOLTIP_GUI_MAP_OWNER)
#define STR_TIP_GUI_MAP_EXTINFO     (STR_TIP_NONE+TOOLTIP_GUI_MAP_EXTINFO)
#define STR_TIP_GUI_MAP_NOLOCK      (STR_TIP_NONE+TOOLTIP_GUI_MAP_NOLOCK)
#define STR_TIP_GUI_MAP_LOCKVWR     (STR_TIP_NONE+TOOLTIP_GUI_MAP_LOCKVWR)
#define STR_TIP_GUI_MAP_LOCKSEL     (STR_TIP_NONE+TOOLTIP_GUI_MAP_LOCKSEL)
#define STR_TIP_GUI_MAP_ZOOMOUT     (STR_TIP_NONE+TOOLTIP_GUI_MAP_ZOOMOUT)
#define STR_TIP_GUI_MAP_ZOOMIN      (STR_TIP_NONE+TOOLTIP_GUI_MAP_ZOOMIN)
#define STR_TIP_GUI_MAP_SIZE        (STR_TIP_NONE+TOOLTIP_GUI_MAP_SIZE)

#define STR_TIP_GUI_FINDER_ACTION_WAIT      (STR_TIP_NONE+TOOLTIP_GUI_FINDER_ACTION)
#define STR_TIP_GUI_FINDER_ACTION_FIGHT     (STR_TIP_NONE+TOOLTIP_GUI_FINDER_ACTION_FIGHT)
#define STR_TIP_GUI_FINDER_ACTION_GOTO      (STR_TIP_NONE+TOOLTIP_GUI_FINDER_ACTION_GOTO)
#define STR_TIP_GUI_FINDER_ACTION_ESCAPE    (STR_TIP_NONE+TOOLTIP_GUI_FINDER_ACTION_ESCAPE)

#define STR_TIP_GUI_ENERGY_RELOAD       (STR_TIP_NONE+TOOLTIP_GUI_ENERGY_RELOAD)
#define STR_TIP_GUI_ENERGY_SYSAKKU      (STR_TIP_NONE+TOOLTIP_GUI_ENERGY_SYSAKKU)
#define STR_TIP_GUI_ENERGY_VHCLAKKU     (STR_TIP_NONE+TOOLTIP_GUI_ENERGY_VHCLAKKU)
#define STR_TIP_GUI_ENERGY_BUILDAKKU    (STR_TIP_NONE+TOOLTIP_GUI_ENERGY_BUILDAKKU)
#define STR_TIP_GUI_ENERGY_BEAMAKKU     (STR_TIP_NONE+TOOLTIP_GUI_ENERGY_BEAMAKKU)

#define STR_TIP_GUI_MODE        (STR_TIP_NONE+TOOLTIP_GUI_MODE)
#define STR_TIP_GUI_WEAPONS     (STR_TIP_NONE+TOOLTIP_GUI_WEAPONS)
#define STR_TIP_GUI_VEHICLES    (STR_TIP_NONE+TOOLTIP_GUI_VEHICLES)
#define STR_TIP_GUI_BUILDINGS   (STR_TIP_NONE+TOOLTIP_GUI_BUILDINGS)
#define STR_TIP_GUI_HUD         (STR_TIP_NONE+TOOLTIP_GUI_HUD)
#define STR_TIP_GUI_CTRL2LM     (STR_TIP_NONE+TOOLTIP_GUI_CTRL2LM)

#define STR_TIP_GUI_FINDER_NUMVHCLS (STR_TIP_NONE+TOOLTIP_GUI_FINDER_NUMVHCLS)
#define STR_TIP_GUI_HELP            (STR_TIP_NONE+TOOLTIP_GUI_HELP)
#define STR_TIP_GUI_ONLINEHELP      (STR_TIP_NONE+TOOLTIP_GUI_ONLINEHELP)

/*** Aktionen ***/
#define STR_TIP_ACTION_SELECT           (STR_TIP_NONE+TOOLTIP_ACTION_SELECT)
#define STR_TIP_ACTION_GOTO             (STR_TIP_NONE+TOOLTIP_ACTION_GOTO)
#define STR_TIP_ACTION_ATTACK_SEC       (STR_TIP_NONE+TOOLTIP_ACTION_ATTACK_SEC)
#define STR_TIP_ACTION_ATTACK_VHCL      (STR_TIP_NONE+TOOLTIP_ACTION_ATTACK_VHCL)
#define STR_TIP_ACTION_NEW              (STR_TIP_NONE+TOOLTIP_ACTION_NEW)
#define STR_TIP_ACTION_ADD              (STR_TIP_NONE+TOOLTIP_ACTION_ADD)
#define STR_TIP_ACTION_CONTROL          (STR_TIP_NONE+TOOLTIP_ACTION_CONTROL)
#define STR_TIP_ACTION_BUILD            (STR_TIP_NONE+TOOLTIP_ACTION_BUILD)
#define STR_TIP_ACTION_BEAM             (STR_TIP_NONE+TOOLTIP_ACTION_BEAM)

/*** Fehlermeldungen ***/
#define STR_TIP_ERROR_NOTARGET          (STR_TIP_NONE+TOOLTIP_ERROR_NOTARGET)
#define STR_TIP_ERROR_NOROOM            (STR_TIP_NONE+TOOLTIP_ERROR_NOROOM)
#define STR_TIP_ERROR_NOENERGY          (STR_TIP_NONE+TOOLTIP_ERROR_NOENERGY)
#define STR_TIP_ERROR_NOTCONQUERED      (STR_TIP_NONE+TOOLTIP_ERROR_NOTCONQUERED)
#define STR_TIP_ERROR_TOOFAR            (STR_TIP_NONE+TOOLTIP_ERROR_TOOFAR)
#define STR_TIP_ERROR_TOOCLOSE          (STR_TIP_NONE+TOOLTIP_ERROR_TOOCLOSE)
#define STR_TIP_ERROR_JUSTBUILDING      (STR_TIP_NONE+TOOLTIP_ERROR_JUSTBUILDING)
#define STR_TIP_ERROR_VHCLSINSECTOR     (STR_TIP_NONE+TOOLTIP_ERROR_VHCLSINSECTOR)
#define STR_TIP_ERROR_TOOJAGGY          (STR_TIP_NONE+TOOLTIP_ERROR_TOOJAGGY)
#define STR_TIP_ERROR_UNKNOWNAREA       (STR_TIP_NONE+TOOLTIP_ERROR_UNKNOWNAREA)

/*** Shellmeldungen ***/
#define STR_TIP_SHELL_PLAY              (STR_TIP_NONE+TOOLTIP_SHELL_PLAY)
#define STR_TIP_SHELL_TUTORIAL          (STR_TIP_NONE+TOOLTIP_SHELL_TUTORIAL)
#define STR_TIP_SHELL_LOCALEOK          (STR_TIP_NONE+TOOLTIP_SHELL_LOCALEOK)
#define STR_TIP_SHELL_LOCALECANCEL      (STR_TIP_NONE+TOOLTIP_SHELL_LOCALECANCEL)
#define STR_TIP_SHELL_NEWLEVEL          (STR_TIP_NONE+TOOLTIP_SHELL_NEWLEVEL)
#define STR_TIP_SHELL_SEND              (STR_TIP_NONE+TOOLTIP_SHELL_SEND)
#define STR_TIP_SHELL_USERRACE          (STR_TIP_NONE+TOOLTIP_SHELL_USERRACE)
#define STR_TIP_SHELL_KYTERNESER        (STR_TIP_NONE+TOOLTIP_SHELL_KYTERNESER)
#define STR_TIP_SHELL_MYKONIER          (STR_TIP_NONE+TOOLTIP_SHELL_MYKONIER)
#define STR_TIP_SHELL_TAERKASTEN        (STR_TIP_NONE+TOOLTIP_SHELL_TAERKASTEN)
#define STR_TIP_SHELL_READY             (STR_TIP_NONE+TOOLTIP_SHELL_READY)
#define STR_TIP_SHELL_NOTREADY          (STR_TIP_NONE+TOOLTIP_SHELL_NOTREADY)
#define STR_TIP_SHELL_LOAD              (STR_TIP_NONE+TOOLTIP_SHELL_LOAD)
#define STR_TIP_SHELL_SAVE              (STR_TIP_NONE+TOOLTIP_SHELL_SAVE)

#define STR_TIP_SHELL_OCINPUT           (STR_TIP_NONE+TOOLTIP_SHELL_OCINPUT)
#define STR_TIP_SHELL_OCVIDEO           (STR_TIP_NONE+TOOLTIP_SHELL_OCVIDEO)
#define STR_TIP_SHELL_OCSOUND           (STR_TIP_NONE+TOOLTIP_SHELL_OCSOUND)
#define STR_TIP_SHELL_OCDISK            (STR_TIP_NONE+TOOLTIP_SHELL_OCDISK)
#define STR_TIP_SHELL_OCLOCALE          (STR_TIP_NONE+TOOLTIP_SHELL_OCLOCALE)
#define STR_TIP_SHELL_PL_PLAY           (STR_TIP_NONE+TOOLTIP_SHELL_PL_PLAY)
#define STR_TIP_SHELL_PL_STOP           (STR_TIP_NONE+TOOLTIP_SHELL_PL_STOP)
#define STR_TIP_SHELL_PL_LOAD           (STR_TIP_NONE+TOOLTIP_SHELL_PL_LOAD)
#define STR_TIP_SHELL_PL_FASTFORWARD    (STR_TIP_NONE+TOOLTIP_SHELL_PL_FASTFORWARD)
#define STR_TIP_SHELL_PL_SETBACK        (STR_TIP_NONE+TOOLTIP_SHELL_PL_SETBACK)
#define STR_TIP_SHELL_PL_GAME           (STR_TIP_NONE+TOOLTIP_SHELL_PL_GAME)
#define STR_TIP_SHELL_QUITGAME          (STR_TIP_NONE+TOOLTIP_SHELL_QUITGAME)
#define STR_TIP_SHELL_QUALIFIER         (STR_TIP_NONE+TOOLTIP_SHELL_QUALIFIER)
#define STR_TIP_SHELL_ENTERINPUT        (STR_TIP_NONE+TOOLTIP_SHELL_ENTERINPUT)
#define STR_TIP_SHELL_PLAYLEFT          (STR_TIP_NONE+TOOLTIP_SHELL_PLAYLEFT)
#define STR_TIP_SHELL_PLAYRIGHT         (STR_TIP_NONE+TOOLTIP_SHELL_PLAYRIGHT)
#define STR_TIP_SHELL_OCNET             (STR_TIP_NONE+TOOLTIP_SHELL_OCNET)
#define STR_TIP_SHELL_HELP              (STR_TIP_NONE+TOOLTIP_SHELL_HELP)
#define STR_TIP_SHELL_PL_TOTITLE        (STR_TIP_NONE+TOOLTIP_SHELL_PL_TOTITLE)
#define STR_TIP_SHELL_PL_TOPLAY         (STR_TIP_NONE+TOOLTIP_SHELL_PL_TOPLAY)
#define STR_TIP_SHELL_NETCANCEL         (STR_TIP_NONE+TOOLTIP_SHELL_NETCANCEL)
#define STR_TIP_SHELL_NETHELP           (STR_TIP_NONE+TOOLTIP_SHELL_NETHELP)
#define STR_TIP_SHELL_OKPROVIDER        (STR_TIP_NONE+TOOLTIP_SHELL_OKPROVIDER)
#define STR_TIP_SHELL_OKPLAYER          (STR_TIP_NONE+TOOLTIP_SHELL_OKPLAYER)
#define STR_TIP_SHELL_OKLEVEL           (STR_TIP_NONE+TOOLTIP_SHELL_OKLEVEL)
#define STR_TIP_SHELL_OKSESSION         (STR_TIP_NONE+TOOLTIP_SHELL_OKSESSION)
#define STR_TIP_SHELL_STARTLEVEL        (STR_TIP_NONE+TOOLTIP_SHELL_STARTLEVEL)
#define STR_TIP_SHELL_BACKTOPROVIDER    (STR_TIP_NONE+TOOLTIP_SHELL_BACKTOPROVIDER)
#define STR_TIP_SHELL_BACKTOPLAYER      (STR_TIP_NONE+TOOLTIP_SHELL_BACKTOPLAYER)
#define STR_TIP_SHELL_BACKTOSESSION     (STR_TIP_NONE+TOOLTIP_SHELL_BACKTOSESSION)


// FIXME: Debriefing Tooltips!

#define STR_TIP_SHELL_DELETE            (STR_TIP_NONE+TOOLTIP_SHELL_DELETE)
#define STR_TIP_SHELL_NEWGAME           (STR_TIP_NONE+TOOLTIP_SHELL_NEWGAME)
#define STR_TIP_SHELL_OKLOAD            (STR_TIP_NONE+TOOLTIP_SHELL_OKLOAD)
#define STR_TIP_SHELL_OKSAVE            (STR_TIP_NONE+TOOLTIP_SHELL_OKSAVE)
#define STR_TIP_SHELL_OKDELETE          (STR_TIP_NONE+TOOLTIP_SHELL_OKDELETE)
#define STR_TIP_SHELL_OKCREATE          (STR_TIP_NONE+TOOLTIP_SHELL_OKCREATE)
#define STR_TIP_SHELL_DISKCANCEL        (STR_TIP_NONE+TOOLTIP_SHELL_DISKCANCEL)
#define STR_TIP_SHELL_CANCELLOAD        (STR_TIP_NONE+TOOLTIP_SHELL_CANCELLOAD)
#define STR_TIP_SHELL_CANCELSAVE        (STR_TIP_NONE+TOOLTIP_SHELL_CANCELSAVE)
#define STR_TIP_SHELL_CANCELDELETE      (STR_TIP_NONE+TOOLTIP_SHELL_CANCELDELETE)
#define STR_TIP_SHELL_CANCELCREATE      (STR_TIP_NONE+TOOLTIP_SHELL_CANCELCREATE)
#define STR_TIP_SHELL_SETTINGSCANCEL    (STR_TIP_NONE+TOOLTIP_SHELL_SETTINGSCANCEL)
#define STR_TIP_SHELL_SETTINGSOK        (STR_TIP_NONE+TOOLTIP_SHELL_SETTINGSOK)
#define STR_TIP_SHELL_RESOLUTION        (STR_TIP_NONE+TOOLTIP_SHELL_RESOLUTION)
#define STR_TIP_SHELL_SOUNDSWITCH       (STR_TIP_NONE+TOOLTIP_SHELL_SOUNDSWITCH)
#define STR_TIP_SHELL_SOUNDLR           (STR_TIP_NONE+TOOLTIP_SHELL_SOUNDLR)
#define STR_TIP_SHELL_FARVIEW           (STR_TIP_NONE+TOOLTIP_SHELL_FARVIEW)
#define STR_TIP_SHELL_HEAVEN            (STR_TIP_NONE+TOOLTIP_SHELL_HEAVEN)
#define STR_TIP_SHELL_FILTERING         (STR_TIP_NONE+TOOLTIP_SHELL_FILTERING)
#define STR_TIP_SHELL_CDSOUND           (STR_TIP_NONE+TOOLTIP_SHELL_CDSOUND)
#define STR_TIP_SHELL_SOFTMOUSE         (STR_TIP_NONE+TOOLTIP_SHELL_SOFTMOUSE)
#define STR_TIP_SHELL_ENEMYINDICATOR    (STR_TIP_NONE+TOOLTIP_SHELL_ENEMYINDICATOR)
#define STR_TIP_SHELL_FXVOLUME          (STR_TIP_NONE+TOOLTIP_SHELL_FXVOLUME)
#define STR_TIP_SHELL_CDVOLUME          (STR_TIP_NONE+TOOLTIP_SHELL_CDVOLUME)
#define STR_TIP_SHELL_FXSLIDER          (STR_TIP_NONE+TOOLTIP_SHELL_FXSLIDER)
#define STR_TIP_SHELL_INPUTOK           (STR_TIP_NONE+TOOLTIP_SHELL_INPUTOK)
#define STR_TIP_SHELL_INPUTCANCEL       (STR_TIP_NONE+TOOLTIP_SHELL_INPUTCANCEL)
#define STR_TIP_SHELL_INPUTRESET        (STR_TIP_NONE+TOOLTIP_SHELL_INPUTRESET)
#define STR_TIP_SHELL_JOYSTICK          (STR_TIP_NONE+TOOLTIP_SHELL_JOYSTICK)
#define STR_TIP_SHELL_FORCEFEEDBACK     (STR_TIP_NONE+TOOLTIP_SHELL_FORCEFEEDBACK)
#define STR_TIP_SHELL_SWITCHOFF         (STR_TIP_NONE+TOOLTIP_SHELL_SWITCHOFF)

/*** Tasten-Bezeichnung ***/
#define STR_KEY_NOP                 (1000)
#define STR_KEY_ESCAPE              (1001)
#define STR_KEY_SPACEBAR            (1002)
#define STR_KEY_CURSOR_UP           (1003)
#define STR_KEY_CURSOR_DOWN         (1004)
#define STR_KEY_CURSOR_LEFT         (1005)
#define STR_KEY_CURSOR_RIGHT        (1006)
#define STR_KEY_F1                  (1007)
#define STR_KEY_F2                  (1008)
#define STR_KEY_F3                  (1009)
#define STR_KEY_F4                  (1010)
#define STR_KEY_F5                  (1011)
#define STR_KEY_F6                  (1012)
#define STR_KEY_F7                  (1013)
#define STR_KEY_F8                  (1014)
#define STR_KEY_F9                  (1015)
#define STR_KEY_F10                 (1016)
#define STR_KEY_F11                 (1017)
#define STR_KEY_F12                 (1018)
#define STR_KEY_BS                  (1019)
#define STR_KEY_TAB                 (1020)
#define STR_KEY_CLEAR               (1021)
#define STR_KEY_RETURN              (1022)
#define STR_KEY_CTRL                (1023)
#define STR_KEY_SHIFT               (1024)
#define STR_KEY_ALT                 (1025)
#define STR_KEY_PAUSE               (1026)
#define STR_KEY_PAGEUP              (1027)
#define STR_KEY_PAGEDOWN            (1028)
#define STR_KEY_END                 (1029)
#define STR_KEY_HOME                (1030)
#define STR_KEY_SELECT              (1031)
#define STR_KEY_EXECUTE             (1032)
#define STR_KEY_SNAPSHOT            (1033)
#define STR_KEY_INS                 (1034)
#define STR_KEY_DEL                 (1035)
#define STR_KEY_HELP                (1036)
#define STR_KEY_1                   (1037)
#define STR_KEY_2                   (1038)
#define STR_KEY_3                   (1039)
#define STR_KEY_4                   (1040)
#define STR_KEY_5                   (1041)
#define STR_KEY_6                   (1042)
#define STR_KEY_7                   (1043)
#define STR_KEY_8                   (1044)
#define STR_KEY_9                   (1045)
#define STR_KEY_0                   (1046)
#define STR_KEY_A                   (1047)
#define STR_KEY_B                   (1048)
#define STR_KEY_C                   (1049)
#define STR_KEY_D                   (1050)
#define STR_KEY_E                   (1051)
#define STR_KEY_F                   (1052)
#define STR_KEY_G                   (1053)
#define STR_KEY_H                   (1054)
#define STR_KEY_I                   (1055)
#define STR_KEY_J                   (1056)
#define STR_KEY_K                   (1057)
#define STR_KEY_L                   (1058)
#define STR_KEY_M                   (1059)
#define STR_KEY_N                   (1060)
#define STR_KEY_O                   (1061)
#define STR_KEY_P                   (1062)
#define STR_KEY_Q                   (1063)
#define STR_KEY_R                   (1064)
#define STR_KEY_S                   (1065)
#define STR_KEY_T                   (1066)
#define STR_KEY_U                   (1067)
#define STR_KEY_V                   (1068)
#define STR_KEY_W                   (1069)
#define STR_KEY_X                   (1070)
#define STR_KEY_Y                   (1071)
#define STR_KEY_Z                   (1072)
#define STR_KEY_NUM_0               (1073)
#define STR_KEY_NUM_1               (1074)
#define STR_KEY_NUM_2               (1075)
#define STR_KEY_NUM_3               (1076)
#define STR_KEY_NUM_4               (1077)
#define STR_KEY_NUM_5               (1078)
#define STR_KEY_NUM_6               (1079)
#define STR_KEY_NUM_7               (1080)
#define STR_KEY_NUM_8               (1081)
#define STR_KEY_NUM_9               (1082)
#define STR_KEY_NUM_MUL             (1083)
#define STR_KEY_NUM_PLUS            (1084)
#define STR_KEY_NUM_DOT             (1085)
#define STR_KEY_NUM_MINUS           (1086)
#define STR_KEY_ENTER               (1087)
#define STR_KEY_NUM_DIV             (1088)
#define STR_KEY_EXTRA_1             (1089)
#define STR_KEY_EXTRA_2             (1090)
#define STR_KEY_EXTRA_3             (1091)
#define STR_KEY_EXTRA_4             (1092)
#define STR_KEY_EXTRA_5             (1093)
#define STR_KEY_EXTRA_6             (1094)
#define STR_KEY_EXTRA_7             (1095)
#define STR_KEY_EXTRA_8             (1096)
#define STR_KEY_EXTRA_9             (1097)
#define STR_KEY_EXTRA_10            (1098)
#define STR_KEY_EXTRA_11            (1099)
#define STR_KEY_LMB                 (1100) // nicht in Tabelle
#define STR_KEY_MMB                 (1101)
#define STR_KEY_RMB                 (1102) // nicht in Tabelle
#define STR_KEY_JB0                 (1103)
#define STR_KEY_JB1                 (1104)
#define STR_KEY_JB2                 (1105)
#define STR_KEY_JB3                 (1106)
#define STR_KEY_JB4                 (1107)
#define STR_KEY_JB5                 (1108)
#define STR_KEY_JB6                 (1109)
#define STR_KEY_JB7                 (1110)

/*** Platz fuer je ueber 256 Vehikel-, Building-, Level-Namen ***/
#define STR_NAME_VEHICLES                               (1200)
#define STR_NAME_BUILDINGS                              (1500)
#define STR_NAME_LEVELS                                 (1800)
#define STR_MISSION_TEXTS                               (2100)

/*** alle restlichen Strings ***/
#define STR_YPAERROR_HEADLINE           (2400)  // ueberschrift einer fehlerbox
#define STR_YPAERROR_NOJOIN             (2401)  // Cannot join session
#define STR_NGADGET_REFRESHSESSIONS     (2402)
#define STR_NGADGET_COMPUTER            (2403)
#define STR_NGADGET_HASOTHERFILES       (2404)
#define STR_YPAERROR_HOSTLATENCY1       (2405)
#define STR_YPAERROR_CLIENTLATENCY1     (2406)
#define STR_YPAERROR_NETSTATUS          (2407)
#define STR_YPAERROR_TIMEDOUT           (2408)
#define STR_YPAERROR_ALLOK              (2409)

#define STR_RACE_ZERO                   (2410)  // allgemeine Rassen-Namen
#define STR_RACE_RESISTANCE             (2411)
#define STR_RACE_SULG                   (2412)                  
#define STR_RACE_MYKO                   (2413)
#define STR_RACE_TAER                   (2414)
#define STR_RACE_BLACK                  (2415)
#define STR_RACE_KYT                    (2416)
#define STR_RACE_NEUTRAL                (2417) 

#define STR_SHELL_EXITDEBRIEFING        (2420) 
#define STR_TIP_SHELL_EXITDEBRIEFING    (2421)
#define STR_SHELL_GOTOLOADSAVE          (2422)
#define STR_YPAERROR_HOSTLATENCY2       (2423)
#define STR_YPAERROR_CLIENTLATENCY2     (2424)
#define STR_YPAERROR_KICKOFF_YOU1       (2425)
#define STR_YPAERROR_KICKOFF_YOU2       (2426)
#define STR_YPAERROR_KICKOFF_PLAYER1    (2427)
#define STR_YPAERROR_KICKOFF_PLAYER2    (2428)
#define STR_YPAERROR_WAITINGFORPLAYER1  (2429)
#define STR_YPAERROR_WAITINGFORPLAYER2  (2430)
#define STR_VGADGET_16BITTEXTURE        (2431)
#define STR_VGADGET_DRAWPRIMITIVE       (2432)
#define STR_IGADGET_ALTJOYSTICK         (2433)


/*** Anpassen! ***/
#define STR_LAST_ITEM           (2600) 

/*-----------------------------------------------------------------*/
#endif

