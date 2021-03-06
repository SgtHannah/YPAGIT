#ifndef NUCLEUS_CONFIG_H
#define NUCLEUS_CONFIG_H
/*
**  $Source: PRG:VFM/Include/nucleus/config.h,v $
**  $Revision: 38.3 $
**  $Date: 1996/11/10 20:48:17 $
**  $Locker:  $
**  $Author: floh $
**
**  Definitionen f�r Nucleus2-Config-Handling.
**
**  (C) Copyright 1995 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

/*-------------------------------------------------------------------
**  How To:
**  ~~~~~~~
**  Um sich ein Set an Konfigurations-Daten aulesen zu lassen,
**  bedient man sich der Nucleus-Funktion nc_GetConfigItems().
**
**  Dieser Funktion �bergibt man einen Filenamen (n�mlich den des
**  Config-Files), ein vorkonfiguriertes ConfigItem-Array und die
**  Anzahl der ConfigItems im ConfigItem-Array.
**  Das ConfigItem-Array enth�lt f�r jedes gesuchte ConfigItem
**  sein Keyword (in ConfigItem.keyword), seinen Datentyp
**  (in ConfigItem.type) und den Default-Wert des Items in
**  (in ConfigItem.data).
**
**  Sonderfall Strings:
**  ~~~~~~~~~~~~~~~~~~~
**  Wenn (ConfigItem.type == CONFIG_STRING) ist, mu�
**  ConfigItem.data auf einen char Buffer zeigen, der gro� genug
**  f�r den gesuchten Config-String sein mu�. Dieser Buffer
**  sollte mit einem Default-String oder NULL vorinitialisiert
**  werden.
**
**  nc_GetConfigItem() kopiert dann bei Bedarf den gefundenen
**  Config-String einfach �ber den Default-String. Dabei kann es
**  nat�rlich passieren, das der String-Buffer �berschrieben wird.
**  Auf der sicheren Seite steht man, wenn man zur Deklaration
**  des Buffers die Konstante CONFIG_MAX_STRING_LEN verwendet
**  (wie definiert in diesem File hier).
*/

/*-------------------------------------------------------------------
**  Dies sind die Pfadnamen der "Default-Config-Files" f�r die
**  verschiedenen Systeme.
*/
#ifdef AMIGA
#define CONFIG_FILE "env:nucleus.prefs"
#endif

#ifdef __DOS__    /* im CurDir */
#define CONFIG_FILE "NUCLEUS.CFG"
#endif

#ifdef __WINDOWS__
#define CONFIG_FILE "NUCLEUS.INI"
#endif

/*-------------------------------------------------------------------
**  maximale L�nge eines Config-Strings
*/
#define CONFIG_MAX_STRING_LEN (128)

/*-------------------------------------------------------------------
**  Aufbau eines "ConfigItems"
*/
struct ConfigItem {
    UBYTE *keyword;         /* C-String mit Keyword */
    ULONG type;             /* siehe unten */
    ULONG data;             /* Datenwert des ConfigItems, oder Pointer */
                            /* auf String-Buffer */
};

/*-------------------------------------------------------------------
**  Folgende Datentypen sind zur Zeit f�r ConfigItem.type
**  definiert:
*/
#define CONFIG_INTEGER  (0)     // eine LONG-Integer-Zahl
#define CONFIG_STRING   (1)     // ein C-String
#define CONFIG_BOOL     (2)     // ein Wahrheits-Wert
#define CONFIG_ROL      (3)     // "Rest Of Line" String

/*-----------------------------------------------------------------*/
#endif

