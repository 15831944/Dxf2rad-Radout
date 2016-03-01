# Microsoft Developer Studio Project File - Name="radout_r15" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=radout_r15 - Win32 Debug
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "radout_r15.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "radout_r15.mak" CFG="radout_r15 - Win32 Debug"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "radout_r15 - Win32 Release" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE "radout_r15 - Win32 Debug" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "radout_r15 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "radout_r"
# PROP BASE Intermediate_Dir "radout_r"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "15r"
# PROP Intermediate_Dir "15r"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "RADOUT15_EXPORTS" /YX /FD /c
# ADD CPP /nologo /G6 /MD /W3 /Gi /GX /O2 /Ob2 /I "..\src\radout" /I "..\src\adslib" /I "..\src\dll" /I "..\src\acis" /I "..\src\geom" /I "$(arx15dir)\utils\brep\inc" /I "$(arx15dir)\inc" /D "NDEBUG" /D "ACRXAPP" /D "_WINDLL" /D "_WINDOWS" /D "R15" /D "ACIS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /win32
# SUBTRACT MTL /mktyplib203
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib advapi32.lib uuid.lib /nologo /dll /machine:I386
# ADD LINK32 bigacad.lib vc5brep.lib /nologo /dll /incremental:yes /machine:I386 /out:"radout_r15.arx" /libpath:"$(ARX15Dir)\lib" /libpath:"$(ARX15Dir)\utils\brep\lib"
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "radout_r15 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "15d"
# PROP Intermediate_Dir "15d"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "RADOUT15_EXPORTS" /YX /FD /c
# ADD CPP /nologo /G6 /MD /W3 /Gi /GX /ZI /Od /I "..\src\radout" /I "..\src\adslib" /I "..\src\dll" /I "..\src\acis" /I "..\src\geom" /I "$(arx15dir)\utils\brep\inc" /I "$(arx15dir)\inc" /D "_DEBUG" /D "ACRXAPP" /D "_WINDLL" /D "_WINDOWS" /D "R15" /D "ACIS" /FR /YX /FD /c
# SUBTRACT CPP /u
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 bigacad.lib vc5brep.lib /nologo /dll /incremental:no /debug /debugtype:both /machine:I386 /out:"radout_r15_d.arx" /pdbtype:sept /libpath:"$(ARX15Dir)\lib" /libpath:"$(ARX15Dir)\utils\brep\lib"
# SUBTRACT LINK32 /pdb:none /map

!ENDIF 

# Begin Target

# Name "radout_r15 - Win32 Release"
# Name "radout_r15 - Win32 Debug"
# Begin Group "src"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\radout\acis_r15.cpp
# End Source File
# Begin Source File

SOURCE=..\src\radout\cproto.h
# End Source File
# Begin Source File

SOURCE=..\src\radout\dialog.c
# End Source File
# Begin Source File

SOURCE=..\src\radout\geom.c
# End Source File
# Begin Source File

SOURCE=..\src\radout\lispapi.c
# End Source File
# Begin Source File

SOURCE=..\src\radout\lispapi.h
# End Source File
# Begin Source File

SOURCE=..\src\radout\lists.c
# End Source File
# Begin Source File

SOURCE=..\src\radout\main.cpp
# End Source File
# Begin Source File

SOURCE=..\src\radout\main_r15.cpp
# End Source File
# Begin Source File

SOURCE=..\src\radout\radout.c
# End Source File
# Begin Source File

SOURCE=..\src\radout\radout.h
# End Source File
# Begin Source File

SOURCE=.\radout_r15.def
# End Source File
# Begin Source File

SOURCE=..\src\radout\smooth.c
# End Source File
# Begin Source File

SOURCE=..\src\radout\utils.c
# End Source File
# End Group
# Begin Group "adslib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\adslib\adcolor.c
# End Source File
# Begin Source File

SOURCE=..\src\radout\adcolor.h
# End Source File
# Begin Source File

SOURCE=..\src\adslib\ads_perr.c
# End Source File
# Begin Source File

SOURCE=..\src\adslib\adtools.h
# End Source File
# Begin Source File

SOURCE=..\src\adslib\error.c
# End Source File
# Begin Source File

SOURCE=..\src\adslib\misc.c
# End Source File
# Begin Source File

SOURCE=..\src\adslib\resbuf.c
# End Source File
# Begin Source File

SOURCE=..\src\adslib\resbuf.h
# End Source File
# End Group
# Begin Group "dll"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\dll\dll.c
# End Source File
# Begin Source File

SOURCE=..\src\dll\dllproto.h
# End Source File
# Begin Source File

SOURCE=..\src\dll\dlltypes.h
# End Source File
# End Group
# Begin Group "acis"

# PROP Default_Filter ""
# End Group
# Begin Group "geom"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\geom\bulge.c
# End Source File
# Begin Source File

SOURCE=..\src\geom\geomdefs.h
# End Source File
# Begin Source File

SOURCE=..\src\geom\geomproto.h
# End Source File
# Begin Source File

SOURCE=..\src\geom\geomtypes.h
# End Source File
# Begin Source File

SOURCE=..\src\geom\m4geom.c
# End Source File
# Begin Source File

SOURCE=..\src\geom\m4inv.c
# End Source File
# Begin Source File

SOURCE=..\src\geom\m4mat.c
# End Source File
# Begin Source File

SOURCE=..\src\geom\m4post.c
# End Source File
# Begin Source File

SOURCE=..\src\geom\poly.c
# End Source File
# Begin Source File

SOURCE=..\src\geom\polycheck.c
# End Source File
# Begin Source File

SOURCE=..\src\geom\v3vec.c
# End Source File
# Begin Source File

SOURCE=..\src\dxf2rad\writerad.h
# End Source File
# Begin Source File

SOURCE=..\src\dxf2rad\writerad.c
# End Source File
# End Group
# End Target
# End Project
