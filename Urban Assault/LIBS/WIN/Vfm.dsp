# Microsoft Developer Studio Project File - Name="VFM" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=VFM - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Vfm.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Vfm.mak" CFG="VFM - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "VFM - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "VFM - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe

!IF  "$(CFG)" == "VFM - Win32 Release"

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

!ELSEIF  "$(CFG)" == "VFM - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Zp1 /w /W0 /GX /Z7 /Od /I "..\..\include" /I "..\..\amiga" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "__NETWORK__" /D "__DBCS__" /D "_WIN32" /D "__WIN32" /D "MSDOS" /D "__WINDOWS__" /D "_LittleEndian_" /FAs /FR /YX /FD /c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "VFM - Win32 Release"
# Name "VFM - Win32 Debug"
# Begin Source File

SOURCE=..\..\classes\areaclas\AC_PUBLE.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\adeclass\ADE_CLAS.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ameshcla\AMESH_PU.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ameshcla\AMESHCLA.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\areaclas\AREACLAS.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\baniclas\BANI_SUP.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\baniclas\BANICLAS.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\baniclas\BANIO.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\baseclas\BASECLAS.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\baseclas\BC_ATTRI.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\baseclas\BC_FAMIL.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\baseclas\BC_LOADS.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\baseclas\BC_TFORM.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\baseclas\BC_TRIGG.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\bitmapcl\BITMAPCL.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\buttoncl\BT_ATTRS.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\buttoncl\BT_BUILD.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\buttoncl\BT_INPUT.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\buttoncl\BT_MAIN.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\buttoncl\BT_PUBLI.C
# End Source File
# Begin Source File

SOURCE=..\..\NUCLEUS2\CONFIG.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\displayc\DISP_MAI.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\embedcla\EMBEDCLA.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ilbmclas\GETFILE.C
# End Source File
# Begin Source File

SOURCE=..\..\ENGINES\TFORM_NG\GETMATRI.C
# End Source File
# Begin Source File

SOURCE=..\..\ENGINES\GFXENGIN\GFX_MAIN.C
# End Source File
# Begin Source File

SOURCE=..\..\ENGINES\TFORM_NG\HIGHLEVE.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\idevclas\ID_MAIN.C
# End Source File
# Begin Source File

SOURCE=..\..\ENGINES\INPUTENG\IE_MAIN.C
# End Source File
# Begin Source File

SOURCE=..\..\NUCLEUS2\PC\IFF.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ilbmclas\ILBMCLAS.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\inputcla\IN_MAIN.C
# End Source File
# Begin Source File

SOURCE=..\..\NUCLEUS2\PC\IO.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\itimercl\IT_MAIN.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\iwimpcla\IW_MAIN.C
# End Source File
# Begin Source File

SOURCE=..\..\NUCLEUS2\PC\LISTS.C
# End Source File
# Begin Source File

SOURCE=..\..\NUCLEUS2\LOG.C
# End Source File
# Begin Source File

SOURCE=..\..\NUCLEUS2\PC\MEMORY.C
# End Source File
# Begin Source File

SOURCE=..\..\NUCLEUS2\NUCLEUS.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\nucclass\NUCLEUSC.C
# End Source File
# Begin Source File

SOURCE=..\..\NUCLEUS2\NUKEDOS.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\networkc\NW_ATTRS.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\networkc\NW_MAIN.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\particle\PL_MAIN.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\particle\PL_RAND.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\particle\PL_SUPPO.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\ilbmclas\PUTFILE.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\reqclass\RQ_ATTRS.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\reqclass\RQ_BUILD.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\reqclass\RQ_INPUT.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\reqclass\RQ_MAIN.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\reqclass\RQ_PUBLI.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\rsrcclas\RSRCCLAS.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\rastercl\RST_BLIT.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\rastercl\RST_MAIN.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\rastercl\RST_POLY.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\rastercl\RST_PRIM.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\rastercl\RST_SPAN.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\rastercl\RST_TEXT.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\samplecl\SAMPLECL.C
# End Source File
# Begin Source File

SOURCE=..\..\NUCLEUS2\PC\SEGMENT.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\skeleton\SKELCLAS.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\skeleton\SKL_CLIP.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\skltclas\SKLTCLAS.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\skltclas\SKLTFILE.C
# End Source File
# Begin Source File

SOURCE=..\..\ENGINES\MILESENG\SND_CDPL.C
# End Source File
# Begin Source File

SOURCE=..\..\ENGINES\MILESENG\SND_MAIN.C
# End Source File
# Begin Source File

SOURCE=..\..\ENGINES\MILESENG\SND_SOUN.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\rastercl\span_x86.asm

!IF  "$(CFG)" == "VFM - Win32 Release"

# Begin Custom Build
InputDir=\anarchy\classes\rastercl
OutDir=.\Release
InputPath=..\..\classes\rastercl\span_x86.asm
InputName=span_x86

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /c /coff /nologo /Cp /DMASM /DSTD_CALL /Zm /Fo$(OutDir)\ /Fl$(OutDir)\\
       /FR$(OutDir)\ /Zi $(InputDir)\$(InputName).asm

# End Custom Build

!ELSEIF  "$(CFG)" == "VFM - Win32 Debug"

# Begin Custom Build
InputDir=\anarchy\classes\rastercl
OutDir=.\Debug
InputPath=..\..\classes\rastercl\span_x86.asm
InputName=span_x86

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /c /coff /nologo /Cp /DMASM /DSTD_CALL /Zm /Fo$(OutDir)\ /Fl$(OutDir)\\
       /FR$(OutDir)\ /Zi $(InputDir)\$(InputName).asm

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\NUCLEUS2\PC\SYSTEM.C
# End Source File
# Begin Source File

SOURCE=..\..\NUCLEUS2\PC\TAGS.C
# End Source File
# Begin Source File

SOURCE=..\..\ENGINES\TFORM_NG\TE_MAIN.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\skeleton\TRANSFOR.C
# End Source File
# Begin Source File

SOURCE=..\..\CLASSES\WIN3DCLA\w3d_ebdp.c
# End Source File
# Begin Source File

SOURCE=..\..\classes\win3dcla\W3D_MAIN.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\win3dcla\W3D_PIXF.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\win3dcla\W3D_POLY.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\win3dcla\W3D_PRIM.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\win3dcla\W3D_TEXT.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\win3dcla\W3D_TXTC.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\win3dcla\W3D_WINB.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\wavclass\WAVCLASS.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\winddcla\WDD_EDIT.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\winddcla\WDD_LOG.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\winddcla\WDD_MAIN.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\winddcla\WDD_TEXT.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\winddcla\WDD_WINB.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\windpcla\WDP_MAIN.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\windpcla\WDP_PLAY.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\windpcla\WDP_TCP.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\windpcla\WDP_WINE.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\windpcla\WDP_WINM.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\windpcla\WDP_WINP.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\windpcla\WDP_WINT.C
# End Source File
# Begin Source File

SOURCE=..\..\CLASSES\WINDPCLA\Wdp_winu.c
# End Source File
# Begin Source File

SOURCE=..\..\CLASSES\WININPCL\winp_log.c
# End Source File
# Begin Source File

SOURCE=..\..\classes\wininpcl\WINP_MAI.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\wininpcl\WINP_WBO.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\wintimer\WINT_MAI.C
# End Source File
# Begin Source File

SOURCE=..\..\classes\wintimer\WINT_WBO.C
# End Source File
# End Target
# End Project
