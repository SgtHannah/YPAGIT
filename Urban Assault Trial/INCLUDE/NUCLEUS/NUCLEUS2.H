#ifndef NUCLEUS_NUCLEUS_H
#define NUCLEUS_NUCLEUS_H
/*
**  $Source: PRG:VFM/Include/nucleus/nucleus2.h,v $
**  $Revision: 38.13 $
**  $Date: 1998/01/06 14:02:34 $
**  $Locker: floh $
**  $Author: floh $
**
**  Schnittstellen-Definition für Nucleus V2.
**
**  (C) Copyright 1994 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif

#ifndef EXEC_LISTS_H
#include <exec/lists.h>
#endif

#ifndef LIBRARIES_IFFPARSE_H
#include <libraries/iffparse.h>
#endif

#ifndef NUCLEUS_SYS_H
#include "nucleus/sys.h"
#endif

#ifndef NUCLEUS_SYNTAX_H
#include "nucleus/syntax.h"
#endif

#ifndef NUCLEUS_SEGMENT_H
#include "nucleus/segment.h"
#endif

#ifndef NUCLEUS_CLASS_H
#include "nucleus/class.h"
#endif

#ifndef NUCLEUS_CONFIG_H
#include "nucleus/config.h"
#endif

#ifndef NUCLEUS_LOG_H
#include "nucleus/log.h"
#endif

#ifndef NUCLEUS_IO_H
#include "nucleus/io.h"
#endif

#ifndef NUCLEUS_NUKEDOS_H
#include "nucleus/nukedos.h"
#endif

/*-------------------------------------------------------------------
**  Die Object-Nodes dienen zur Object-internen Verkettung.
*/
struct ObjectNode {
    struct MinNode Node;
    Object *Object;
};

struct ObjectPriorityNode {
    struct MinNode Node;
    Object *Object;
    LONG Priority;
};

/*-------------------------------------------------------------------
**  NucleusBase Struktur, ist global definiert in nucleus.c,
**  und kann bei der Initialisierung von externen
**  Modulen mit Hilfe des Modul-Tags MID_NucleusBase
**  (def in modules.h) angefordert werden.
*/
#define NC_NUM_CFG_OVERRIDE (32)

struct NucleusBase {

    struct MinList ClassList;

    /*** Diagnostics ***/
    LONG AllocCount;
    LONG AllocSize;
    LONG AllocMax;
    LONG FOpenCount;

    struct GET_Engine *VisOutEng_GET;
    struct GET_Engine *AudOutEng_GET;
    struct GET_Engine *InEng_GET;
    struct GET_Engine *TransEng_GET;

    struct SegmentHandle VisOutEng_SH;
    struct SegmentHandle AudOutEng_SH;
    struct SegmentHandle InEng_SH;
    struct SegmentHandle TransEng_SH;

    /*** SysPaths ***/
    UBYTE SysPath[NC_NUM_SYSPATHS][256];

    /*** Config-Overrides ***/
    UBYTE *CfgItem[NC_NUM_CFG_OVERRIDE];
    ULONG CfgIndex;

    /*** Public Object Handles ***/
    Object *GfxObject;      // init by gfx driver class
    Object *InpObject;      // init by input driver class

    /*** File IO ***/
    struct MinList AssignList;  // struct ncAssign Nodes
};

/*-------------------------------------------------------------------
**  Das Nucleus V2 "API" beinhaltet alles, um Klassen-Dispatcher
**  möglichst systemunabhängig zu implementieren. Außer den
**  Filefunktionen (ANSI C) richten sich alle Parameter-Templates
**  nach den gleichnamigen AmigaOS-Funktionen (die Amiga-Version
**  von Nucleus ist in der Tat meistens eine direkte Umlenkung 
**  ins AmigaOS).
*/
#ifdef AMIGA        /* Amiga-speziell: die Register-Parameter */
struct GET_Nucleus {

    /*** Kernel Funktionen V2.0 ***/

    /* memory v2.0 */
    APTR __asm (*AllocVec)(__d0 ULONG, __d1 ULONG);
    void __asm (*FreeVec) (__a1 APTR);

    /* lists v2.0 */
    void __asm (*NewList) (__a0 struct List *);
    void __asm (*AddHead) (__a0 struct List *, __a1 struct Node *);
    void __asm (*AddTail) (__a0 struct List *, __a1 struct Node *);
    struct Node * __asm   (*RemHead) (__a0 struct List *);
    struct Node * __asm   (*RemTail) (__a0 struct List *);
    void __asm (*Remove)  (__a1 struct Node *);
    struct Node * __asm (*FindName) (__a0 struct List *, __a1 STRPTR);

    /* io v2.0 */
    APTR __asm (*FOpen)  (__a0 STRPTR, __a1 STRPTR); /* ANSI */
    LONG __asm (*FClose) (__a0 APTR);                /* ANSI */
    LONG __asm (*FSeek)  (__a0 APTR, __d0 ULONG, __d1 ULONG);    /* ANSI */
    LONG __asm (*FRead)  (__a0 APTR, __d0 ULONG, __d1 ULONG, __a1 APTR);/* ANSI */
    LONG __asm (*FWrite) (__a0 APTR, __d0 ULONG, __d1 ULONG, __a1 APTR);/* ANSI */

    /* tags v2.0 */
    struct TagItem *__asm (*FindTagItem) (__d0 ULONG, __a0 struct TagItem *);
    ULONG __asm (*GetTagData) (__d0 ULONG, __d1 ULONG, __a0 struct TagItem *);

    /* iff v2.0 */
    struct IFFHandle * __asm (*AllocIFF) (void);
    void __asm (*FreeIFF)     (__a0 struct IFFHandle *);
    void __asm (*InitIFFasNucleus) (__a0 struct IFFHandle *);
    LONG __asm (*OpenIFF)     (__a0 struct IFFHandle *, __d0 LONG);
    void __asm (*CloseIFF)    (__a0 struct IFFHandle *);
    LONG __asm (*PushChunk)   (__a0 struct IFFHandle *, __d0 LONG, __d1 LONG, __d2 LONG);
    LONG __asm (*PopChunk)    (__a0 struct IFFHandle *);
    LONG __asm (*ReadChunkBytes)  (__a0 struct IFFHandle *, __a1 APTR, __d0 LONG);
    LONG __asm (*WriteChunkBytes) (__a0 struct IFFHandle *, __a1 APTR, __d0 LONG);
    LONG __asm (*ParseIFF)    (__a0 struct IFFHandle *, __d0 LONG control);
    struct ContextNode * __asm (*CurrentChunk) (__a0 struct IFFHandle *);
    BOOL __asm (*SkipChunk)   (__a0 struct IFFHandle *);    /* NON STANDARD */

    /*** Engines v2.0 ***/
    struct GET_Engine *__asm (*OpenEngine) (__d0 ULONG);
    void __asm (*CloseEngine)              (__d0 ULONG);

    /*** Object Nucleus v2.0 ***/
    Object *(*NewObject)   (STRPTR, ULONG,...);          /* Stack Args!!! */
    BOOL __asm (*DisposeObject)  (__a0 Object *);
    ULONG (*DoMethod)      (Object *, ULONG, ULONG,...); /* Stack Args!!! */
    ULONG __asm (*DoMethodA)     (__a0 Object *, __d0 ULONG, __a2 Msg *);
    ULONG __asm (*DoSuperMethodA) (__a1 Class *, __a0 Object *, __d0 ULONG, __a2 Msg *);

    /*** Persistent Nucleus v2.0 ***/
    BOOL __asm (*SaveObject)    (__a0 Object *, __a1 STRPTR);
    Object *__asm (*LoadObject) (__a0 STRPTR);
    BOOL __asm (*SaveInnerObject) (__a0 Object *, __a1 struct IFFHandle *);
    Object *__asm (*LoadInnerObject) (__a0 struct IFFHandle *);

    /*** Config Handling v2.01 ***/
    BOOL __asm (*GetConfigItems) (__a0 STRPTR, __a1 struct ConfigItem *, __d0 ULONG);

    /*** io v2.01 ***/
    STRPTR __asm (*FGetS) (__a0 STRPTR, __d0 ULONG, __a1 APTR);

    /*** log v2.02 ***/
    void __asm (*SetLogMode) (__d0 ULONG mode);
    void (*LogMsg) (STRPTR, ...);

    /*** system dos paths ***/
    void __asm (*SetSysPath) (__d0 ULONG, __a0 STRPTR);
    STRPTR __asm (*GetSysPath) (__d0 ULONG);

    /*** io v2.1 ***/
    __asm ULONG (*FDelete)(__a0 STRPTR);
    __asm APTR (*FOpenDir)(__a0 STRPTR);
    __asm void (*FCloseDir)(__a0 STRPTR);
    __asm struct ncDirEntry *(*FReadDir)(__a0 APTR, __a1 struct ncDirEntry *);
    __asm ULONG (*FMakeDir)(__a0 UBYTE *);
    __asm ULONG (*FRemDir)(__a0 UBYTE *);

    /*** Config Handling v2.01 ***/
    __asm BOOL (*ConfigOverride)(__a0 STRPTR);
};
/*-----------------------------------------------------------------*/
#else       /* Rest der Welt: ANSI-Prototypes */
/*-----------------------------------------------------------------*/
struct GET_Nucleus {

    /*** Kernel Funktionen V2.0 ***/

    /* memory v2.0 */
    APTR (*AllocVec)(ULONG, ULONG);
    void (*FreeVec) (APTR);

    /* lists v2.0 */
    void (*NewList) (struct List *);
    void (*AddHead) (struct List *, struct Node *);
    void (*AddTail) (struct List *, struct Node *);
    struct Node *(*RemHead) (struct List *);
    struct Node *(*RemTail) (struct List *);
    void (*Remove)  (struct Node *);
    struct Node *(*FindName) (struct List *, STRPTR);

    /* io v2.0 */
    APTR  (*FOpen)  (STRPTR, STRPTR);           /* ANSI */
    LONG (*FClose) (APTR);                     /* ANSI */
    LONG (*FSeek)  (APTR, ULONG, ULONG);       /* ANSI */
    LONG (*FRead)  (APTR, ULONG, ULONG, APTR); /* ANSI */
    LONG (*FWrite) (APTR, ULONG, ULONG, APTR); /* ANSI */

    /* tags v2.0 */
    struct TagItem *(*FindTagItem) (ULONG, struct TagItem *);
    ULONG (*GetTagData) (ULONG, ULONG, struct TagItem *);

    /* iff v2.0 */
    struct IFFHandle *(*AllocIFF) (void);
    void (*FreeIFF)     (struct IFFHandle *);
    void (*InitIFFasNucleus) (struct IFFHandle *);
    LONG (*OpenIFF)     (struct IFFHandle *, LONG);
    void (*CloseIFF)    (struct IFFHandle *);
    LONG (*PushChunk)   (struct IFFHandle *, LONG, LONG, LONG);
    LONG (*PopChunk)    (struct IFFHandle *);
    LONG (*ReadChunkBytes)  (struct IFFHandle *, APTR, LONG);
    LONG (*WriteChunkBytes) (struct IFFHandle *, APTR, LONG);
    LONG (*ParseIFF)    (struct IFFHandle *, LONG control);
    struct ContextNode *(*CurrentChunk) (struct IFFHandle *);
    BOOL (*SkipChunk)   (struct IFFHandle *);    /* NON STANDARD */

    /*** Engines v2.0 ***/
    struct GET_Engine *(*OpenEngine) (ULONG);
    void (*CloseEngine)              (ULONG);

    /*** Object Nucleus v2.0 ***/
    Object *(*NewObject)   (STRPTR, ULONG,...);
    BOOL (*DisposeObject)  (Object *);
    ULONG (*DoMethod)      (Object *, ULONG, ULONG,...);
    ULONG (*DoMethodA)     (Object *, ULONG, Msg *);
    ULONG (*DoSuperMethodA) (Class *, Object *, ULONG, Msg *);

    /*** Persistent Nucleus v2.0 ***/
    BOOL (*SaveObject)    (Object *, STRPTR);
    Object *(*LoadObject) (STRPTR);
    BOOL (*SaveInnerObject) (Object *, struct IFFHandle *);
    Object *(*LoadInnerObject) (struct IFFHandle *);

    /*** Config Handling v2.01 ***/
    BOOL (*GetConfigItems) (STRPTR, struct ConfigItem *, ULONG);

    /*** io v2.01 ***/
    STRPTR (*FGetS) (STRPTR, ULONG, APTR);

    /*** log v2.02 ***/
    void (*SetLogMode) (ULONG mode);
    void (*LogMsg) (STRPTR, ...);

    /*** system dos paths ***/
    void (*SetSysPath) (ULONG, STRPTR);
    STRPTR (*GetSysPath) (ULONG);

    /*** io v2.1 ***/
    ULONG (*FDelete)(STRPTR);
    APTR (*FOpenDir)(STRPTR);
    void (*FCloseDir)(STRPTR);
    struct ncDirEntry *(*FReadDir)(APTR, struct ncDirEntry *);
    ULONG (*FMakeDir)(UBYTE *);
    ULONG (*FRemDir)(UBYTE *);

    /*** Config Handling v2.01 ***/
    BOOL (*ConfigOverride)(STRPTR);
};
#endif

/*-------------------------------------------------------------------
**  Unabhängig von der Einsprungtabelle hier nochmal die
**  "normalen" Prototypes, für Direkt-Zugriff von Modulen
**  aus, die statisch gelinkt sind. Hier sind auch
**  interne "undokumentierte" Funktionen zu finden.
*/
#ifdef AMIGA

struct GET_Nucleus *nc_OpenNucleus(void);
void nc_CloseNucleus(void);

APTR __asm nc_AllocVec(__d0 ULONG, __d1 ULONG);
void __asm nc_FreeVec(__a1 APTR);

void __asm nc_NewList(__a0 struct List *);
void __asm nc_AddHead(__a0 struct List *, __a1 struct Node *);
void __asm nc_AddTail(__a0 struct List *, __a1 struct Node *);
struct Node *__asm nc_RemHead(__a0 struct List *);
struct Node *__asm nc_RemTail(__a0 struct List *);
void __asm nc_Remove(__a1 struct Node *);
struct Node *__asm nc_FindName(__a0 struct List *, __a1 STRPTR);

APTR __asm nc_FOpen(__a0 STRPTR, __a1 STRPTR); /* ANSI */
LONG __asm nc_FClose(__a0 APTR);                /* ANSI */
LONG __asm nc_FSeek(__a0 APTR, __d0 ULONG, __d1 ULONG);    /* ANSI */
LONG __asm nc_FRead(__a0 APTR, __d0 ULONG, __d1 ULONG, __a1 APTR);/* ANSI */
LONG __asm nc_FWrite(__a0 APTR, __d0 ULONG, __d1 ULONG, __a1 APTR);/* ANSI */

struct TagItem *__asm nc_FindTagItem(__d0 ULONG, __a0 struct TagItem *);
ULONG __asm nc_GetTagData(__d0 ULONG, __d1 ULONG, __a0 struct TagItem *);

struct IFFHandle *nc_AllocIFF(void);
void __asm nc_FreeIFF(__a0 struct IFFHandle *);
void __asm nc_InitIFFasNucleus(__a0 struct IFFHandle *);
LONG __asm nc_OpenIFF(__a0 struct IFFHandle *, __d0 LONG);
void __asm nc_CloseIFF(__a0 struct IFFHandle *);
LONG __asm nc_PushChunk(__a0 struct IFFHandle *, __d0 LONG, __d1 LONG, __d2 LONG);
LONG __asm nc_PopChunk(__a0 struct IFFHandle *);
LONG __asm nc_ReadChunkBytes(__a0 struct IFFHandle *, __a1 APTR, __d0 LONG);
LONG __asm nc_WriteChunkBytes(__a0 struct IFFHandle *, __a1 APTR, __d0 LONG);
LONG __asm nc_ParseIFF(__a0 struct IFFHandle *, __d0 LONG);
struct ContextNode *__asm nc_CurrentChunk(__a0 struct IFFHandle *);
BOOL __asm nc_SkipChunk(__a0 struct IFFHandle *);    /* NON STANDARD */

struct GET_Engine *__asm nc_OpenEngine(__d0 ULONG);
void __asm nc_CloseEngine(__d0 ULONG);

Object *nc_NewObject(STRPTR, ULONG,...);          /* Stack Args!!! */
BOOL __asm nc_DisposeObject(__a0 Object *);
ULONG nc_DoMethod(Object *, ULONG, ULONG,...); /* Stack Args!!! */
ULONG __asm nc_DoMethodA(__a0 Object *, __d0 ULONG, __a2 Msg *);
ULONG __asm nc_DoSuperMethodA(__a1 Class *, __a0 Object *, __d0 ULONG, __a2 Msg *);

BOOL __asm nc_SaveObject(__a0 Object *, __a1 STRPTR);
Object *__asm nc_LoadObject(__a0 STRPTR);
BOOL __asm nc_SaveInnerObject(__a0 Object *, __a1 struct IFFHandle *);
Object *__asm nc_LoadInnerObject(__a0 struct IFFHandle *);

BOOL __asm nc_GetConfigItems(__a0 STRPTR, __a1 struct ConfigItem *, __d0 ULONG);
STRPTR __asm nc_FGetS(__a0 STRPTR, __d0 ULONG, __a1 APTR);

void __asm nc_SetLogMode(__d0 ULONG mode);
void nc_LogMsg(STRPTR, ...);

/*** PRIVATE ***/
ULONG *__asm nc_OpenSegment(__a0 UBYTE *, __a1 struct SegmentHandle *);
void __asm nc_CloseSegment(__a0 struct SegmentHandle *);
BOOL nc_SystemInit(void);
void nc_SystemCleanup(void);
Class *nc_MakeClass(STRPTR);
BOOL nc_FreeClass(Class *);

BOOL __asm nc_InitLog(__a0 UBYTE *channel);
void nc_KillLog(void);

/*** system dos paths ***/
void __asm nc_SetSysPath(__d0 ULONG, __a0 STRPTR);
STRPTR __asm nc_GetSysPath(__d0 ULONG);

/*** io v2.1 ***/
__asm ULONG nc_FDelete(__a0 STRPTR);
__asm APTR nc_FOpenDir(__a0 STRPTR);
__asm void nc_FCloseDir(__a0 STRPTR);
__asm struct ncDirEntry *nc_FReadDir(__a0 APTR, __a1 struct ncDirEntry *);
__asm ULONG nc_FMakeDir(__a0 UBYTE *);
__asm ULONG nc_FRemDir(__a0 UBYTE *);

/*** Config Handling v2.01 ***/
__asm BOOL nc_ConfigOverride(__a0 STRPTR);

/*-----------------------------------------------------------------*/
#else
/*-----------------------------------------------------------------*/

struct GET_Nucleus *nc_OpenNucleus(void);
void nc_CloseNucleus(void);

APTR nc_AllocVec(ULONG, ULONG);
void nc_FreeVec(APTR);

void nc_NewList(struct List *);
void nc_AddHead(struct List *, struct Node *);
void nc_AddTail(struct List *, struct Node *);
struct Node *nc_RemHead(struct List *);
struct Node *nc_RemTail(struct List *);
void nc_Remove(struct Node *);
struct Node *nc_FindName(struct List *, STRPTR);

APTR nc_FOpen(STRPTR, STRPTR); /* ANSI */
LONG nc_FClose(APTR);                /* ANSI */
LONG nc_FSeek(APTR, ULONG, ULONG);    /* ANSI */
LONG nc_FRead(APTR, ULONG, ULONG, APTR);/* ANSI */
LONG nc_FWrite(APTR, ULONG, ULONG, APTR);/* ANSI */

struct TagItem *nc_FindTagItem(ULONG, struct TagItem *);
ULONG nc_GetTagData(ULONG, ULONG, struct TagItem *);

struct IFFHandle *nc_AllocIFF(void);
void nc_FreeIFF(struct IFFHandle *);
void nc_InitIFFasNucleus(struct IFFHandle *);
LONG nc_OpenIFF(struct IFFHandle *, LONG);
void nc_CloseIFF(struct IFFHandle *);
LONG nc_PushChunk(struct IFFHandle *, LONG, LONG, LONG);
LONG nc_PopChunk(struct IFFHandle *);
LONG nc_ReadChunkBytes(struct IFFHandle *, APTR, LONG);
LONG nc_WriteChunkBytes(struct IFFHandle *, APTR, LONG);
LONG nc_ParseIFF(struct IFFHandle *, LONG);
struct ContextNode *nc_CurrentChunk(struct IFFHandle *);
BOOL nc_SkipChunk(struct IFFHandle *);    /* NON STANDARD */

struct GET_Engine *nc_OpenEngine(ULONG);
void nc_CloseEngine(ULONG);

Object *nc_NewObject(STRPTR, ULONG,...);          /* Stack Args!!! */
BOOL nc_DisposeObject(Object *);
ULONG nc_DoMethod(Object *, ULONG, ULONG,...); /* Stack Args!!! */
ULONG nc_DoMethodA(Object *, ULONG, Msg *);
ULONG nc_DoSuperMethodA(Class *, Object *, ULONG, Msg *);

BOOL nc_SaveObject(Object *, STRPTR);
Object *nc_LoadObject(STRPTR);
BOOL nc_SaveInnerObject(Object *, struct IFFHandle *);
Object *nc_LoadInnerObject(struct IFFHandle *);

BOOL nc_GetConfigItems(STRPTR, struct ConfigItem *, ULONG);
STRPTR nc_FGetS(STRPTR, ULONG, APTR);

void nc_SetLogMode(ULONG mode);
void nc_LogMsg(STRPTR, ...);

/*** PRIVATE ***/
ULONG *nc_OpenSegment(UBYTE *, struct SegmentHandle *);
void nc_CloseSegment(struct SegmentHandle *);
BOOL nc_SystemInit(void);
void nc_SystemCleanup(void);
Class *nc_MakeClass(STRPTR);
BOOL nc_FreeClass(Class *);
BOOL nc_InitLog(UBYTE *channel);
void nc_KillLog(void);

/*** system dos paths ***/
void nc_SetSysPath(ULONG, STRPTR);
STRPTR nc_GetSysPath(ULONG);

/*** io v2.1 ***/
ULONG nc_FDelete(STRPTR);
APTR nc_FOpenDir(STRPTR);
void nc_FCloseDir(STRPTR);
struct ncDirEntry *nc_FReadDir(APTR, struct ncDirEntry *);
ULONG nc_FMakeDir(UBYTE *);
ULONG nc_FRemDir(UBYTE *);

/*** Config Handling v2.01 ***/
BOOL nc_ConfigOverride(STRPTR);

/*** io v2.2 ***/
ULONG nc_SetAssign(STRPTR assign, STRPTR path);
STRPTR nc_GetAssign(STRPTR assign);
void nc_ManglePath(UBYTE *fn, UBYTE *buf, ULONG buf_len);

#ifdef STATIC_LINKING
void nc_AddSegment(struct SegInfo *);
#endif

#endif

/*-----------------------------------------------------------------*/
#endif

