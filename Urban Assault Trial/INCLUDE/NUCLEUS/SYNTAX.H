#ifndef NUCLEUS_SYNTAX_H
#define NUCLEUS_SYNTAX_H
/*
**  $Source: PRG:VFM/Include/nucleus/syntax.h,v $
**  $Revision: 38.15 $
**  $Date: 1998/01/06 14:02:57 $
**  $Locker: floh $
**  $Author: floh $
**
**  Hier ist der neue Nucleus2-Syntax definiert, der die
**  Klassen-Dispatcher-Quelltexte lesbarer macht und auﬂerdem
**  deren Portierung auf andere Maschinen vereinfacht.
**
**  (C) Copyright 1994 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef NUCLEUS_SYS_H
#include "nucleus/sys.h"
#endif

#ifndef NUCLEUS_CLASS_H
#include "nucleus/class.h"
#endif

/*-----------------------------------------------------------------**
**  Nucleus-Kernel-Funktionen                                      **
**-----------------------------------------------------------------*/

/***/
#ifdef DYNAMIC_LINKING

#define _AllocVec           NUC_GET->AllocVec
#define _FreeVec            NUC_GET->FreeVec

#define _NewList            NUC_GET->NewList
#define _AddHead            NUC_GET->AddHead
#define _AddTail            NUC_GET->AddTail
#define _RemHead            NUC_GET->RemHead
#define _RemTail            NUC_GET->RemTail
#define _Remove             NUC_GET->Remove
#define _FindName           NUC_GET->FindName

#define _FOpen              NUC_GET->FOpen
#define _FClose             NUC_GET->FClose
#define _FSeek              NUC_GET->FSeek
#define _FWrite             NUC_GET->FWrite
#define _FRead              NUC_GET->FRead
#define _FGetS              NUC_GET->FGetS

#define _FindTagItem        NUC_GET->FindTagItem
#define _GetTagData         NUC_GET->GetTagData

#define _SetLogMode         NUC_GET->SetLogMode
#define _LogMsg             NUC_GET->LogMsg

#define _AllocIFF           NUC_GET->AllocIFF
#define _FreeIFF            NUC_GET->FreeIFF
#define _InitIFFasNucleus   NUC_GET->InitIFFasNucleus
#define _OpenIFF            NUC_GET->OpenIFF
#define _CloseIFF           NUC_GET->CloseIFF
#define _PushChunk          NUC_GET->PushChunk
#define _PopChunk           NUC_GET->PopChunk
#define _ReadChunkBytes     NUC_GET->ReadChunkBytes
#define _WriteChunkBytes    NUC_GET->WriteChunkBytes
#define _ParseIFF           NUC_GET->ParseIFF
#define _CurrentChunk       NUC_GET->CurrentChunk
#define _SkipChunk          NUC_GET->SkipChunk

#define _GetConfigItems     NUC_GET->GetConfigItems
#define _ConfigOverride     NUC_GET->ConfigOverride

#define _SetSysPath         NUC_GET->SetSysPath
#define _GetSysPath         NUC_GET->GetSysPath

#define _OpenEngine         NUC_GET->OpenEngine
#define _CloseEngine        NUC_GET->CloseEngine

#define _NewObject          NUC_GET->NewObject
#define _DisposeObject      NUC_GET->DisposeObject
#define _DoMethod           NUC_GET->DoMethod
#define _DoMethodA          NUC_GET->DoMethodA
#define _DoSuperMethodA     NUC_GET->DoSuperMethodA

#define _SaveObject         NUC_GET->SaveObject
#define _LoadObject         NUC_GET->LoadObject
#define _SaveInnerObject    NUC_GET->SaveInnerObject
#define _LoadInnerObject    NUC_GET->LoadInnerObject

#define _FDelete            NUC_GET->FDelete
#define _FOpenDir           NUC_GET->FOpenDir
#define _FCloseDir          NUC_GET->FCloseDir
#define _FReadDir           NUC_GET->FReadDir
#define _FMakeDir           NUC_GET->FMakeDir
#define _FRemDir            NUC_GET->FRemDir

#define _Assign(x,y)
#define _GetAssign(x)       (" ")
#endif
/***/
#ifdef STATIC_LINKING

#define _AllocVec           nc_AllocVec
#define _FreeVec            nc_FreeVec

#define _NewList            nc_NewList
#define _AddHead            nc_AddHead
#define _AddTail            nc_AddTail
#define _RemHead            nc_RemHead
#define _RemTail            nc_RemTail
#define _Remove             nc_Remove
#define _FindName           nc_FindName

#define _FOpen              nc_FOpen
#define _FClose             nc_FClose
#define _FSeek              nc_FSeek
#define _FWrite             nc_FWrite
#define _FRead              nc_FRead
#define _FGetS              nc_FGetS

#define _FindTagItem        nc_FindTagItem
#define _GetTagData         nc_GetTagData

#define _SetLogMode         nc_SetLogMode
#define _LogMsg             nc_LogMsg

#define _AllocIFF           nc_AllocIFF
#define _FreeIFF            nc_FreeIFF
#define _InitIFFasNucleus   nc_InitIFFasNucleus
#define _OpenIFF            nc_OpenIFF
#define _CloseIFF           nc_CloseIFF
#define _PushChunk          nc_PushChunk
#define _PopChunk           nc_PopChunk
#define _ReadChunkBytes     nc_ReadChunkBytes
#define _WriteChunkBytes    nc_WriteChunkBytes
#define _ParseIFF           nc_ParseIFF
#define _CurrentChunk       nc_CurrentChunk
#define _SkipChunk          nc_SkipChunk

#define _GetConfigItems     nc_GetConfigItems
#define _ConfigOverride     nc_ConfigOverride

#define _SetSysPath         nc_SetSysPath
#define _GetSysPath         nc_GetSysPath

#define _OpenEngine         nc_OpenEngine
#define _CloseEngine        nc_CloseEngine

#define _NewObject          nc_NewObject
#define _DisposeObject      nc_DisposeObject
#define _DoMethod           nc_DoMethod
#define _DoMethodA          nc_DoMethodA
#define _DoSuperMethodA     nc_DoSuperMethodA

#define _SaveObject         nc_SaveObject
#define _LoadObject         nc_LoadObject
#define _SaveInnerObject    nc_SaveInnerObject
#define _LoadInnerObject    nc_LoadInnerObject

#define _FDelete            nc_FDelete
#define _FOpenDir           nc_FOpenDir
#define _FCloseDir          nc_FCloseDir
#define _FReadDir           nc_FReadDir
#define _FMakeDir           nc_FMakeDir
#define _FRemDir            nc_FRemDir

#define _SetAssign          nc_SetAssign
#define _GetAssign          nc_GetAssign
#define _ManglePath         nc_ManglePath
#endif

/*-----------------------------------------------------------------**
**  Nucleus-OOP-Calls
**  ~~~~~~~~~~~~~~~~~
**  Das d¸rfte die Lesbarkeit der Quelltexte doch ganz erheblich
**  erhˆhen, auﬂerdem sorgt es f¸r weniger Schmerzen in 
**  "thy typing fingers".
**
**  Leider ist es nicht mˆglich, das Setzen/Getten mehrerer Attribute ¸ber
**  Stackframes zu vereinfachen, in diesem Fall _DoMethod() oder
**  _seta/_geta verwenden.
*/
#define _new                _NewObject
#define _dispose            _DisposeObject
#ifndef NUCLEUS_BOOPSI_FRIENDLY
    #define _set(o,attr,val)    _DoMethod(o,OM_SET,attr,val,TAG_DONE)
    #define _get(o,attr,ptr)    _DoMethod(o,OM_GET,attr,ptr,TAG_DONE)
    #define _seta(o,ptr)        _DoMethodA(o,OM_SET,ptr)
    #define _geta(o,ptr)        _DoMethodA(o,OM_SET,ptr)
#else
    #define _set(o,attr,val)    _DoMethod(o,NOM_SET,attr,val,TAG_DONE)
    #define _get(o,attr,ptr)    _DoMethod(o,NOM_GET,attr,ptr,TAG_DONE)
    #define _seta(o,ptr)        _DoMethodA(o,NOM_SET,ptr)
    #define _geta(o,ptr)        _DoMethodA(o,NOM_SET,ptr)
#endif
#define _save(o,name)       _SaveObject(o,name)
#define _load(name)         _LoadObject(name)

#define _do                 _DoMethod
#define _doa                _DoMethodA
#define _dosupera           _DoSuperMethodA

/*** Alternative zu "_do"... ***/
#define _method             _DoMethod
// #define _methoda         _DoMethodA

/*** _methoda() jetzt als Macro ***/
#define _methoda(o,id,msg) (*(((struct nucleusdata *)o)->o_Class->methods[id].dispatcher))(o,(((struct nucleusdata *)o)->o_Class->methods[id].trueclass),msg)

#define _supermethoda       _DoSuperMethodA

/*** allgemeines Engine-Interface ***/
#ifdef DYNAMIC_LINKING
#define _OVE_SetAttrs OVE_GET->SetAttrs
#define _IE_SetAttrs  IE_GET->SetAttrs
#define _TE_SetAttrs  TE_GET->SetAttrs
#define _AE_SetAttrs  AE_GET->SetAttrs
#define _OVE_GetAttrs OVE_GET->GetAttrs
#define _IE_GetAttrs  IE_GET->GetAttrs
#define _TE_GetAttrs  TE_GET->GetAttrs
#define _AE_GetAttrs  AE_GET->GetAttrs
#else
extern void ove_SetAttrs(ULONG,...);
extern void ie_SetAttrs(ULONG,...);
extern void te_SetAttrs(ULONG,...);
extern void audio_SetAttrs(ULONG,...);
extern void ove_GetAttrs(ULONG,...);
extern void ie_GetAttrs(ULONG,...);
extern void te_GetAttrs(ULONG,...);
extern void audio_GetAttrs(ULONG,...);
#define _OVE_SetAttrs ove_SetAttrs
#define _IE_SetAttrs  ie_SetAttrs
#define _TE_SetAttrs  te_SetAttrs
#define _AE_SetAttrs  audio_SetAttrs
#define _OVE_GetAttrs ove_GetAttrs
#define _IE_GetAttrs  ie_GetAttrs
#define _TE_GetAttrs  te_GetAttrs
#define _AE_GetAttrs  audio_GetAttrs
#endif

/*-------------------------------------------------------------------
**  Abstraktions-Macros f¸r Klassen-Dispatcher
*/
#define _get_nbase(tag_list,ptr) *ptr=(struct NucleusBase *)_GetTagData(MID_NucleusBase,0L,tag_list)

#ifdef AMIGA
#define _dispatcher(Result,Name,Msg) __geta4 Result __asm Name(__a0 Object *o, __a1 Class *cl, __a2 Msg)
#else
#define _dispatcher(Result,Name,Msg) Result Name(Object *o, Class *cl, Msg)
#endif

#ifdef DYNAMIC_LINKING
#define _use_nucleus        struct GET_Nucleus *NUC_GET=NULL;
#define _use_ov_engine      struct GET_Engine *OVE_GET=NULL; struct ove_GET_Specific *OVE_GET_SPEC=NULL;
#define _use_tform_engine   struct GET_Engine *TE_GET=NULL; struct teng_GET_Specific *TE_GET_SPEC=NULL;
#define _use_input_engine   struct GET_Engine *IE_GET=NULL; struct ie_GET_Specific *IE_GET_SPEC=NULL;
#define _use_audio_engine   struct GET_Engine *AE_GET=NULL; struct audio_GET_Specific *AE_GET_SPEC=NULL;

#define _extern_use_nucleus         extern struct GET_Nucleus *NUC_GET;
#define _extern_use_ov_engine       extern struct GET_Engine *OVE_GET; extern struct ove_GET_Specific *OVE_GET_SPEC;
#define _extern_use_tform_engine    extern struct GET_Engine *TE_GET; extern struct teng_GET_Specific *TE_GET_SPEC;
#define _extern_use_input_engine    extern struct GET_Engine *IE_GET; extern struct ie_GET_Specific *IE_GET_SPEC;
#define _extern_use_audio_engine    extern struct GET_Engine *AE_GET; extern struct audio_GET_Specific *AE_GET_SPEC;

#define _get_ov_engine(tag_list)    OVE_GET = (struct GET_Engine *) _GetTagData(MID_ENGINE_OUTPUT_VISUAL, 0L, tag_list);  if (OVE_GET) OVE_GET->GetAttrs(OVET_GET_SPEC, &OVE_GET_SPEC, TAG_DONE);
#define _get_tform_engine(tag_list) TE_GET = (struct GET_Engine *) _GetTagData(MID_ENGINE_TRANSFORM, 0L, tag_list);  if (TE_GET) TE_GET->GetAttrs(TET_GET_SPEC, &TE_GET_SPEC, TAG_DONE);
#define _get_input_engine(tag_list) IE_GET = (struct GET_Engine *) _GetTagData(MID_ENGINE_INPUT, 0L, tag_list);  if (IE_GET) IE_GET->GetAttrs(IET_GET_SPEC, &IE_GET_SPEC, TAG_DONE);
#define _get_audio_engine(tag_list) AE_GET = (struct GET_Engine *) _GetTagData(MID_ENGINE_OUTPUT_AUDIO, 0L, tag_list); if (AE_GET) AE_GET->GetAttrs(AET_GET_SPEC, &AE_GET_SPEC, TAG_DONE);
#endif

#ifdef STATIC_LINKING
#define _use_nucleus ;
#define _use_ov_engine ;
#define _use_tform_engine ;
#define _use_input_engine ;
#define _use_audio_engine ;

#define _extern_use_nucleus ;
#define _extern_use_ov_engine ;
#define _extern_use_tform_engine ;
#define _extern_use_input_engine ;
#define _extern_use_audio_engine ;

#define _get_ov_engine(tag_list) ;
#define _get_tform_engine(tag_list) ;
#define _get_input_engine(tag_list) ;
#define _get_audio_engine(tag_list) ;
#endif

/*-----------------------------------------------------------------*/
#endif




