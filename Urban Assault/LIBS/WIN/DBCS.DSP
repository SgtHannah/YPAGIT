# Microsoft Developer Studio Project File - Name="DBCS" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=DBCS - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "DBCS.MAK".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "DBCS.MAK" CFG="DBCS - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "DBCS - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "DBCS - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""$/Urban Assault/LIBS/WIN", NDBAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe

!IF  "$(CFG)" == "DBCS - Win32 Release"

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

!ELSEIF  "$(CFG)" == "DBCS - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DBCS___W"
# PROP BASE Intermediate_Dir "DBCS___W"
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

# Name "DBCS - Win32 Release"
# Name "DBCS - Win32 Debug"
# Begin Source File

SOURCE=H:\dxmedia\include\control.h
# End Source File
# Begin Source File

SOURCE=..\..\INCLUDE\MISC\DBCS.H
# End Source File
# Begin Source File

SOURCE=..\..\ENGINES\DBCS\DBCSMAIN.C
# End Source File
# Begin Source File

SOURCE=..\..\ENGINES\DBCS\DSHOW.C
# End Source File
# Begin Source File

SOURCE=..\..\INCLUDE\MISC\Dshow.h
# End Source File
# Begin Source File

SOURCE=H:\dxmedia\include\evcode.h
# End Source File
# Begin Source File

SOURCE=H:\dxmedia\include\ksuuids.h
# End Source File
# Begin Source File

SOURCE=H:\dxmedia\include\strmif.h
# End Source File
# Begin Source File

SOURCE=H:\dxmedia\include\uuids.h
# End Source File
# End Target
# End Project
