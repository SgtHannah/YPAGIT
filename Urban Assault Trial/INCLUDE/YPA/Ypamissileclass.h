#ifndef YPA_MISSILECLASS_H
#define YPA_MISSILECLASS_H
/*
**  $Source: PRG:VFM/Include/ypa/ypamissileclass.h,v $
**  $Revision: 38.1 $
**  $Date: 1998/01/06 14:27:33 $
**  $Locker:  $
**  $Author: floh $
**
**  Alle autonomen Waffen
**
**  (C) Copyright 1995 by Andreas Flemming
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_LISTS_H
#include <exec/lists.h>
#endif

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#ifndef NUCLEUS_NUCLEUSCLASS_H
#include "nucleus/nucleusclass.h"
#endif

#ifndef ADE_ADE_CLASS_H     /* für ArgStack/PubStack-Entry-Zeug */
#include "ade/ade_class.h"
#endif

#ifndef BASECLASS_BASECLASS_H
#include "baseclass/baseclass.h"
#endif

#ifndef YPA_BACTERIUM_H
#include "ypa/bacterium.h"
#endif

#ifndef YPA_BACTCLASS_H
#include "ypa/ypabactclass.h"
#endif

/*-------------------------------------------------------------------
**  NAME
**      ypamissile.class --  Raketen u.ä. Da diese langlebig sind, dürfen sie
**              nicht an ihrem Schützen hängen, da dieser zeitigersterben kann.
**              Vorerst werden sie als Commander dem WO übergeben, bis eine 
**              bessere Lösung gefunden ist. Damit sie nicht ihren Schützen
**              zerstören, muß dessen Bakterienpointer mit übergeben werden.
**
**
**  FUNCTION
**  METHODS
**      nucleus.class
**      ~~~~~~~~~~~~~
**      ypabact.class
**      ~~~~~~~~~~~~~
**          YBM_AI_LEVEL3     wird hier überladen und an das Fahrverhalten 
**                            angepaßt
**
**          YBM_MOVE          Ebenso.
**
**      ypamissile.class
**      ~~~~~~~~~~~~~~~~
**
**          YMM_RESETVIEWER
**              Wenn die Rakete Viewer war, dann wird der RifleMan auf
**              den Viewer gesetzt. Sinnvoll beim Tod der rakete oder des
**              Trägers
**
**
**
**  ATTRIBUTES
**      nucleus.class
**      ~~~~~~~~~~~~~
**      ypabact.class
**      ~~~~~~~~~~~~~
**      ypazepp.class
**      ~~~~~~~~~~~~~
**
**
*/
#define YPAMISSILE_CLASSID "ypamissile.class"


/*-----------------------------------------------------------------*/
#define YMM_BASE            (YBM_BASE+METHOD_DISTANCE)
#define YMM_RESETVIEWER     (YMM_BASE)
#define YMM_DOIMPULSE       (YMM_BASE+1)
#define YMM_ALIGNMISSILE_S  (YMM_BASE+2)
#define YMM_ALIGNMISSILE_V  (YMM_BASE+3)



/*-----------------------------------------------------------------*/
#define YMA_BASE            (YBA_BASE+ATTRIB_DISTANCE)
#define YMA_RifleMan        (YMA_BASE)
#define YMA_Handle          (YMA_BASE+2)
#define YMA_AutoNode        (YMA_BASE+3)
#define YMA_LifeTime        (YMA_BASE+4)
#define YMA_Delay           (YMA_BASE+5)
#define YMA_DriveTime       (YMA_BASE+6)
#define YMA_IgnoreBuildings (YMA_BASE+7)
#define YMA_EnergyHeli      (YMA_BASE+8)       // SG bei Energie 1000fachen
#define YMA_EnergyTank      (YMA_BASE+9)       // Wert uebergeben
#define YMA_EnergyFlyer     (YMA_BASE+10)
#define YMA_EnergyRobo      (YMA_BASE+11)
#define YMA_RadiusHeli      (YMA_BASE+12)
#define YMA_RadiusTank      (YMA_BASE+13)
#define YMA_RadiusFlyer     (YMA_BASE+14)
#define YMA_RadiusRobo      (YMA_BASE+15)
#define YMA_StartHeight     (YMA_BASE+16)


/*-------------------------------------------------------------------
**  Defaults für Attribute
*/
#define YMA_LifeTime_DEF            5000
#define YMA_DriveTime_DEF           50000

/*** weils überschrieben wird ***/
#define YBA_YourLastSeconds_Missy_DEF (3000)    // für Raketen

/*-----------------------------------------------------------------*/
struct ypamissile_data {

    Object      *world;             // für schnellen Zugriff
    struct ypaworld_data *ywd;
    struct Bacterium *bact;         // für schnelleren Zugriff
    UBYTE       handle;             // Flags für Handhabung
    
    struct Bacterium *rifle_man;    // mein Schütze

    struct OBNode auto_node;        // fürs Einklinken
    LONG        life_time;          // Lebenszeit --> danach Explosion
    LONG        drive_time;         // Triebwerksbrennzeit --> danach Bombe
    LONG        delay;              // nach Aufschlag --> Mine
    ULONG       flags;
    FLOAT       startheight;        // von da falle ich, nur fuer Bomben interessant
    
    /* -----------------------------------------------------
    ** Opfer-spezifische Werte, wenn nix zutrifft, dann Wert
    ** aus Bacterium-Struktur 
    ** ---------------------------------------------------*/
    FLOAT       energy_heli;        // bactClass       
    FLOAT       energy_tank;        // tank & car  
    FLOAT       energy_flyer;       // flyer & UFO
    FLOAT       energy_robo;        // robo
    
    FLOAT       radius_heli;        // ebenso
    FLOAT       radius_tank;
    FLOAT       radius_flyer;
    FLOAT       radius_robo;
};

/*-------------------------------------------------------------------
**  Definitionen für ypamissile_data.flags
*/
#define YMF_Viewer          (1L<<0)     // damit es schnell geht...
#define YMF_CountDelay      (1L<<1)     // für Verzögerung
#define YMF_IgnoreBuildings (1L<<2)     // Zerstört keine WTypes...

/*-------------------------------------------------------------------
**  Definitionen für ypamissile_data.handle
*/

#define YMF_Bomb            1       // Bombe, also MOVE_NOFORCE-Waffe
#define YMF_Simple          2       // ohne Zielsuche
#define YMF_Search          3       // mit Zielsuche
#define YMF_Grenade         4       // ohne Ausgleich der Gravitation
#define YMF_Intelligent     5       // mit Ausweichen bei Hindernissen
#define YMF_Mine            6       // nur für interne Verwendung! Minen
                                    // haben Delay > 0



/*-------------------------------------------------------------------
** noch Zeuch eben
*/

struct alignmissile_msg {

    struct flt_triple vec;      // in ...V
    FLOAT frame_time;           // in ...S
};


#define START_SPEED         70.0   // Startgeschwindigkeit
/*-----------------------------------------------------------------*/
#endif

