# Microsoft Developer Studio Project File - Name="YPA" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=YPA - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "YPA.MAK".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "YPA.MAK" CFG="YPA - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "YPA - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "YPA - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe

!IF  "$(CFG)" == "YPA - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Zp1 /w /W0 /GX /O2 /I "..\..\include" /I "..\..\amiga" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "__NETWORK__" /D "__DBCS__" /D "_WIN32" /D "__WIN32" /D "MSDOS" /D "__WINDOWS__" /D "_LittleEndian_" /YX /FD /c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "YPA - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "YPA___Wi"
# PROP BASE Intermediate_Dir "YPA___Wi"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Zp1 /w /W0 /GX /Z7 /Od /I "..\..\include" /I "..\..\amiga" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "__NETWORK__" /D "__DBCS__" /D "_WIN32" /D "__WIN32" /D "MSDOS" /D "__WINDOWS__" /D "_LittleEndian_" /FR /YX /FD /c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "YPA - Win32 Release"
# Name "YPA - Win32 Debug"
# Begin Source File

SOURCE=..\..\classes\ypabact\YB_AI12.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypabact\YB_AI3.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypabact\YB_ATTRS.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypabact\YB_BACTC.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypabact\YB_CONDI.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypabact\YB_DIE.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypabact\YB_ENERG.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypabact\YB_FIGHT.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypabact\YB_FINDP.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypabact\YB_MAIN.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypabact\YB_MASTE.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypabact\YB_MOVE.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypabact\YB_NETWO.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypabact\YB_POSIT.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypabact\YB_SUPPO.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypabact\YB_TARGE.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypabact\YB_THINK.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypabact\YB_TRIGG.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypabact\YB_USER.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypacar\YC_ATTRS.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypacar\YC_INTEL.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypacar\YC_MAIN.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypacar\YC_MOVE.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaflyer\YF_ATTRS.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaflyer\YF_INTEL.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaflyer\YF_MAIN.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaflyer\YF_MOVE.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaflyer\YF_POSIT.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypagun\YG_ATTRS.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypagun\YG_DIE.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypagun\YG_ENERG.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypagun\YG_FIGHT.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypagun\YG_INTEL.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypagun\YG_MAIN.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypagun\YG_POSIT.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypamissi\YM_ATTRS.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypamissi\YM_CONDI.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypamissi\YM_INTEL.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypamissi\YM_MAIN.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypamissi\YM_SUPPO.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\yparobo\YR_ATTRS.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\yparobo\YR_CHECK.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\yparobo\YR_DIE.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\yparobo\YR_ENERG.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\yparobo\YR_FIGHT.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\yparobo\YR_INTEL.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\yparobo\YR_MAIN.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\yparobo\YR_MESSA.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\yparobo\YR_MOVE.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\yparobo\YR_POSIT.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\yparobo\YR_REACT.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\yparobo\YR_SUPPO.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\yparobo\YR_USER.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypatank\YT_ATTRS.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypatank\YT_BACTC.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypatank\YT_FIGHT.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypatank\YT_INTEL.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypatank\YT_MAIN.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypatank\YT_MOVE.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypatank\YT_POSIT.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaufo\YU_ATTRS.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaufo\YU_FIGHT.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaufo\YU_INTEL.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaufo\YU_MAIN.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaufo\YU_MOVE.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaufo\YU_POSIT.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_ABORT.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_ANALY.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_ATTRI.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_BGSHE.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_BUILD.C
# End Source File
# Begin Source File

SOURCE=..\..\CLASSES\YPAWORLD\yw_catch.c
# End Source File
# Begin Source File

SOURCE=..\..\CLASSES\YPAWORLD\yw_confi.c
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_DEATH.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_DEBRI.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_DESIG.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_ENBAR.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_ENERG.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_ENWIN.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_FINDE.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_FONT.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_FORCE.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_GAMEL.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_GSACT.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_GSINI.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_GSIO.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_GSLAY.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\yw_gsprotos.h
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_GSTRI.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_GUI.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_HISTO.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_HUD.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_INTER.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_JOBS.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_LEVEL.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_LISTV.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_LOCAL.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_LOGWI.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_MAIN.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_MAPRE.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_MAPRN.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_MAPVH.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_MISSI.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_NETIN.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\yw_netprotos.h
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_NETRE.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_NETSU.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_NETUP.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_NETWO.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_NEWIN.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_PARSE.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_PLAY.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_PRLDF.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_PROFI.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\yw_protos.h
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_RADAR.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_RECOR.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_RENDE.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_SELEC.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_SIM.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_STATU.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_SUPER.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_SUPP.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_TOOLT.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_VEHIC.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_VOICE.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_WEAPO.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ypaworld\YW_WINLO.C
# End Source File
# End Target
# End Project
