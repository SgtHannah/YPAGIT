#ifndef NUCLEUS_SYS_H
#define NUCLEUS_SYS_H
/*
**  $Source: PRG:VFM/Include/nucleus/sys.h,v $
**  $Revision: 38.5 $
**  $Date: 1998/01/06 14:03:07 $
**  $Locker:  $
**  $Author: floh $
**
**  Hier vegetieren systemabhängige Standard-Definition vor sich
**  hin, mit denen VFM-Applikation Gebrauch machen sollten,
**  um 100% portabel zu sein (wenigstens auf Quelltext-Ebene).
**
**  (C) Copyright 1994 by A.Weissflog
*/

/*** SAS C Kompatibilität ***/
#ifdef _M68000
#define mc68000
#endif

#ifdef __SASC

#define __d0 register __d0
#define __d1 register __d1
#define __d2 register __d2
#define __d3 register __d3
#define __d4 register __d4
#define __d5 register __d5
#define __d6 register __d6
#define __d7 register __d7

#define __a0 register __a0
#define __a1 register __a1
#define __a2 register __a2
#define __a3 register __a3
#define __a4 register __a4
#define __a5 register __a5
#define __a6 register __a6

#define __geta4 __saveds

#endif

/*** DICE Kompatibilität ***/
#ifdef _DCC
#define __asm
#endif

// #ifdef _M_IX86
// #define __WINDOWS__
// #define __386__
// #define __VISUALC__
// #endif

/*-------------------------------------------------------------------
**  vordefinierte Symbole
**  ~~~~~~~~~~~~~~~~~~~~~
**  Ziel-Plattform-Macros
**      AMIGA                   -> Amiga als Zielplattform
**      MSDOS oder __DOS__      -> 32-Bit-MSDOS
**      __WINDOWS__             -> 32-Bit-Windows
**      __OS2__                 -> OS/2 als Zielplattform
**
**  Calling Conventions
**      __CDECL
**
**  Link-Modell
**      DYNAMIC_LINKING     -> z.B. auf Amiga
**      STATIC_LINKING      -> z.B. unter MSDOS
**
**  Ziel-CPU:
**      mc68000 -> 680x0-Familie    (C-Compiler predefined)
**      __386__ -> i386 kompatibel  (C-Compiler predefined)
**
**  #?Endian
**      _LittleEndian_  -> Ziel-CPU ist Little Endian (z.B. i86)
**      _BigEndian_     -> Ziel-CPU ist Big Endian (z.B. mc68k)
**
**  Verkrüppelte Filenamen:
**
**      _DOS_PATH_STYLE_
**
**      Hauptsächlich die File-IO-Routinen müssen Endian-Konvertierung
**      machen. Alle Nucleus-Files sind _BigEndian_, alles, was
**      nicht <Byte> ist, muß also konvertiert werden!
*/

/*** Watcom betrachtet alle Win32-Sachen als NT ***/
#ifdef __NT__
#define __WINDOWS__
#endif

#ifdef mc68000
#define _BigEndian_
#endif

#ifdef __386__
#define _LittleEndian_
#endif

#ifdef MSDOS
#define _DOS_PATH_STYLE_
#endif
#ifdef __WINDOWS__
#define _DOS_PATH_STYLE_
#endif

#ifdef __VISUALC__
#define __CDECL __cdecl
#else
#define __CDECL
#endif

#ifdef __WINDOWS__
    // #define __NETWORK__
    #ifndef WIN32
    #define WIN32
    #endif
    #ifndef _WIN32
    #define _WIN32
    #endif
    #ifndef __WIN32
    #define __WIN32
    #endif
#endif

/*-------------------------------------------------------------------
**  Link-Modell
**  ~~~~~~~~~~~
*/
#ifdef AMIGA
#define DYNAMIC_LINKING
#endif

#ifdef MSDOS
#define STATIC_LINKING
#endif
#ifdef __WINDOWS__
#define STATIC_LINKING
#endif

/*-------------------------------------------------------------------
**  Macros zur Konvertierung zwischen dem VFM-File-Zahlen-Format
**  _BigEndian_ ins "native" CPU-Format.
**  Der Name der Macros erklärt sich so:
**
**      v2n[lw]     -> VFM-To-Native (LONG, WORD)
**      n2v[lw]     -> Native-To-VFM (LONG, WORD)
**
**  Anwendung z.B. so:
**
**      v2nl(&native_x);
*/
#ifdef _LittleEndian_

#define v2nl(x) { UBYTE typsl;\
                  typsl=((UBYTE *)x)[0];\
                  ((UBYTE *)x)[0]=((UBYTE *)x)[3];\
                  ((UBYTE *)x)[3]=typsl;\
                  typsl=((UBYTE *)x)[1];\
                  ((UBYTE *)x)[1]=((UBYTE *)x)[2];\
                  ((UBYTE *)x)[2]=typsl; };
#define n2vl(x) v2nl(x)

#define v2nw(x) { UBYTE typsl;\
                  typsl=((UBYTE *)x)[0];\
                  ((UBYTE *)x)[0]=((UBYTE *)x)[1];\
                  ((UBYTE *)x)[1]=typsl; };
#define n2vw(x) v2nw(x)
#endif

/****/

#ifdef _BigEndian_
#define v2nl(x) ;
#define n2vl(x) ;
#define v2nw(x) ;
#define n2vw(x) ;
#endif

/*-----------------------------------------------------------------*/
#endif

