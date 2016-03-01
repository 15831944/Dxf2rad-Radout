# Microsoft Developer Studio Project File - Name="radout_ic2004" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=radout_ic2004 - Win32 Debug
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "radout_ic2004.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "radout_ic2004.mak" CFG="radout_ic2004 - Win32 Debug"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "radout_ic2004 - Win32 Release" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE "radout_ic2004 - Win32 Debug" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "radout_ic2004 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ic2004r"
# PROP Intermediate_Dir "ic2004r"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\src\radout" /I "..\src\adslib" /I "..\srd\dll" /I "..\srd\geom" /I "$(ic2004dir)" /D "NDEBUG" /D "_WINDOWS" /D "WIN32" /D "IC2004" /D "SDS_MEMORY" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 sds.lib /nologo /subsystem:windows /dll /machine:I386 /out:"radout_ic2004.dll" /libpath:"$(ic2004dir)"

!ELSEIF  "$(CFG)" == "radout_ic2004 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "ic2004d"
# PROP Intermediate_Dir "ic2004d"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\src\radout" /I "..\srd\AdsLib" /I "..\srd\dll" /I "..\srd\geom" /I "$(ic2004dir)" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "IC2004" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 sds.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"radout_ic2004_d.dll" /pdbtype:sept /libpath:"$(ic2004dir)"
# SUBTRACT LINK32 /verbose

!ENDIF 

# Begin Target

# Name "radout_ic2004 - Win32 Release"
# Name "radout_ic2004 - Win32 Debug"
# Begin Group "src"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\radout\cproto.h
# End Source File
# Begin Source File

SOURCE=..\src\radout\dialog.c
# End Source File
# Begin Source File

SOURCE=..\src\radout\entpoint.c
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

SOURCE=..\src\radout\main_ic2004.cpp
# End Source File
# Begin Source File

SOURCE=..\src\radout\radout.c
# End Source File
# Begin Source File

SOURCE=..\src\radout\radout.h
# End Source File
# Begin Source File

SOURCE=..\src\radout\smooth.c
# End Source File
# Begin Source File

SOURCE=..\src\radout\utils.c
# End Source File
# End Group
# Begin Group "AdsLib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\adslib\adcolor.c
# End Source File
# Begin Source File

SOURCE=..\src\adslib\adcolor.h
# End Source File
# Begin Source File

SOURCE=..\src\adslib\ads_perr.c
# End Source File
# Begin Source File

SOURCE=..\src\adslib\adtools.h
# End Source File
# Begin Source File

SOURCE=..\src\adslib\cadcompat.h
# End Source File
# Begin Source File

SOURCE=..\src\adslib\error.c
# End Source File
# Begin Source File

SOURCE=..\src\AdsLib\misc.c
# End Source File
# Begin Source File

SOURCE=..\src\AdsLib\resbuf.c
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

SOURCE=..\src\dxf2rad\writerad.c
# End Source File
# Begin Source File

SOURCE=..\src\dxf2rad\writerad.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\radout_ic2004.def
# End Source File
# End Target
# End Project
