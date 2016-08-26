# Microsoft Developer Studio Project File - Name="OmniApp2" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=OmniApp2 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "OmniApp2.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "OmniApp2.mak" CFG="OmniApp2 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "OmniApp2 - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "OmniApp2 - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "OmniApp2 - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\__release"
# PROP Intermediate_Dir "..\__release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /G6 /Zp16 /MD /W3 /GX /O2 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x809 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 cv.lib ipl.lib highgui.lib ijl15.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "OmniApp2 - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\__debug"
# PROP Intermediate_Dir "..\__debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /Zp16 /MDd /w /W0 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x809 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ipl.lib cvd.lib ijl15.lib highguid.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "OmniApp2 - Win32 Release"
# Name "OmniApp2 - Win32 Debug"
# Begin Group "gui"

# PROP Default_Filter ""
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\gui\bitmaps\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=.\gui\res\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=.\gui\bitmaps\bmp00001.bmp
# End Source File
# Begin Source File

SOURCE=.\gui\bitmaps\bmp_imag.bmp
# End Source File
# Begin Source File

SOURCE=.\gui\bitmaps\bmp_spla.bmp
# End Source File
# Begin Source File

SOURCE=.\gui\bitmaps\clock.bmp
# End Source File
# Begin Source File

SOURCE=.\gui\bitmaps\default.bmp
# End Source File
# Begin Source File

SOURCE=.\gui\bitmaps\float_wi.bmp
# End Source File
# Begin Source File

SOURCE=.\gui\res\ico00001.ico
# End Source File
# Begin Source File

SOURCE=.\gui\res\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\gui\res\idr_smal.ico
# End Source File
# Begin Source File

SOURCE=.\gui\res\OmniApp2.ico
# End Source File
# Begin Source File

SOURCE=.\gui\res\OmniApp2.rc2
# End Source File
# Begin Source File

SOURCE=.\gui\bitmaps\splash.bmp
# End Source File
# Begin Source File

SOURCE=.\gui\res\Toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\gui\res\view_bar.bmp
# End Source File
# End Group
# Begin Source File

SOURCE=.\gui\BkgView.cpp
# End Source File
# Begin Source File

SOURCE=.\gui\BkgView.h
# End Source File
# Begin Source File

SOURCE=.\gui\CameraViewList.cpp
# End Source File
# Begin Source File

SOURCE=.\gui\CameraViewList.h
# End Source File
# Begin Source File

SOURCE=.\gui\FloatingImageView.cpp
# End Source File
# Begin Source File

SOURCE=.\gui\FloatingImageView.h
# End Source File
# Begin Source File

SOURCE=.\gui\FloatingOmniImageView.cpp
# End Source File
# Begin Source File

SOURCE=.\gui\FloatingOmniImageView.h
# End Source File
# Begin Source File

SOURCE=.\gui\FloatingPanoramaView.cpp
# End Source File
# Begin Source File

SOURCE=.\gui\FloatingPanoramaView.h
# End Source File
# Begin Source File

SOURCE=.\gui\FloatingPerspectiveView.cpp
# End Source File
# Begin Source File

SOURCE=.\gui\FloatingPerspectiveView.h
# End Source File
# Begin Source File

SOURCE=.\gui\Ipl2Bmp.cpp
# End Source File
# Begin Source File

SOURCE=.\gui\Ipl2Bmp.h
# End Source File
# Begin Source File

SOURCE=.\gui\Ipl2DecimatedBmp.cpp
# End Source File
# Begin Source File

SOURCE=.\gui\Ipl2DecimatedBmp.h
# End Source File
# Begin Source File

SOURCE=.\gui\LogView.cpp
# End Source File
# Begin Source File

SOURCE=.\gui\LogView.h
# End Source File
# Begin Source File

SOURCE=.\gui\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\gui\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\gui\OmniApp2.cpp
# End Source File
# Begin Source File

SOURCE=.\gui\OmniApp2.h
# End Source File
# Begin Source File

SOURCE=.\gui\OmniApp2.rc
# End Source File
# Begin Source File

SOURCE=.\gui\Resource.h
# End Source File
# Begin Source File

SOURCE=.\gui\SplashWindow.cpp
# End Source File
# Begin Source File

SOURCE=.\gui\SplashWindow.h
# End Source File
# Begin Source File

SOURCE=.\gui\StaticImageView.cpp
# End Source File
# Begin Source File

SOURCE=.\gui\StaticImageView.h
# End Source File
# Begin Source File

SOURCE=.\gui\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\gui\StdAfx.h
# End Source File
# End Group
# Begin Group "framework"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\framework\Colour2GrayNode.h
# End Source File
# Begin Source File

SOURCE=.\framework\CorrectAspectRatio.cpp
# End Source File
# Begin Source File

SOURCE=.\framework\CorrectAspectRatio.h
# End Source File
# Begin Source File

SOURCE=.\framework\framework.h
# End Source File
# Begin Source File

SOURCE=.\framework\Gray2ColourNode.cpp
# End Source File
# Begin Source File

SOURCE=.\framework\Gray2ColourNode.h
# End Source File
# Begin Source File

SOURCE=.\framework\IplData.cpp
# End Source File
# Begin Source File

SOURCE=.\framework\IplData.h
# End Source File
# Begin Source File

SOURCE=.\framework\IplImageResource.cpp
# End Source File
# Begin Source File

SOURCE=.\framework\IplImageResource.h
# End Source File
# Begin Source File

SOURCE=.\framework\MemoryPool.cpp
# End Source File
# Begin Source File

SOURCE=.\framework\MemoryPool.h
# End Source File
# Begin Source File

SOURCE=.\framework\MemoryResource.cpp
# End Source File
# Begin Source File

SOURCE=.\framework\MemoryResource.h
# End Source File
# Begin Source File

SOURCE=.\framework\ProcessorNode.cpp
# End Source File
# Begin Source File

SOURCE=.\framework\ProcessorNode.h
# End Source File
# Begin Source File

SOURCE=.\framework\ReadJpegSequence.cpp
# End Source File
# Begin Source File

SOURCE=.\framework\ReadJpegSequence.h
# End Source File
# Begin Source File

SOURCE=.\framework\RunnableObject.cpp
# End Source File
# Begin Source File

SOURCE=.\framework\RunnableObject.h
# End Source File
# Begin Source File

SOURCE=.\framework\SlideshowViewerNode.cpp
# End Source File
# Begin Source File

SOURCE=.\framework\SlideshowViewerNode.h
# End Source File
# Begin Source File

SOURCE=.\framework\SyncNode.cpp
# End Source File
# Begin Source File

SOURCE=.\framework\SyncNode.h
# End Source File
# Begin Source File

SOURCE=.\framework\ViewerNode.cpp
# End Source File
# Begin Source File

SOURCE=.\framework\ViewerNode.h
# End Source File
# Begin Source File

SOURCE=.\framework\WriteBmpSequence.cpp
# End Source File
# Begin Source File

SOURCE=.\framework\WriteBmpSequence.h
# End Source File
# End Group
# Begin Group "core"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\core\BivariateGaussian.cpp
# End Source File
# Begin Source File

SOURCE=.\core\BivariateGaussian.h
# End Source File
# Begin Source File

SOURCE=.\core\BkgSubtracter.cpp
# End Source File
# Begin Source File

SOURCE=.\core\BkgSubtracter.h
# End Source File
# Begin Source File

SOURCE=.\core\Blob.cpp
# End Source File
# Begin Source File

SOURCE=.\core\Blob.h
# End Source File
# Begin Source File

SOURCE=.\core\BlobTracker.cpp
# End Source File
# Begin Source File

SOURCE=.\core\BlobTracker.h
# End Source File
# Begin Source File

SOURCE=.\core\BoundaryDetector.cpp
# End Source File
# Begin Source File

SOURCE=.\core\BoundaryDetector.h
# End Source File
# Begin Source File

SOURCE=.\core\CfgParameters.cpp
# End Source File
# Begin Source File

SOURCE=.\core\CfgParameters.h
# End Source File
# Begin Source File

SOURCE=.\core\ColourTracker.cpp
# End Source File
# Begin Source File

SOURCE=.\core\ColourTracker.h
# End Source File
# Begin Source File

SOURCE=.\core\EdgeDetector.cpp
# End Source File
# Begin Source File

SOURCE=.\core\EdgeDetector.h
# End Source File
# Begin Source File

SOURCE=.\core\EM.cpp
# End Source File
# Begin Source File

SOURCE=.\core\EM.h
# End Source File
# Begin Source File

SOURCE=.\core\HistoryNode.cpp
# End Source File
# Begin Source File

SOURCE=.\core\HistoryNode.h
# End Source File
# Begin Source File

SOURCE=.\core\IterativeThreshold.cpp
# End Source File
# Begin Source File

SOURCE=.\core\IterativeThreshold.h
# End Source File
# Begin Source File

SOURCE=.\core\Log.cpp
# End Source File
# Begin Source File

SOURCE=.\core\Log.h
# End Source File
# Begin Source File

SOURCE=.\core\MatchScores.cpp
# End Source File
# Begin Source File

SOURCE=.\core\MatchScores.h
# End Source File
# Begin Source File

SOURCE=.\core\MixtureModel.cpp
# End Source File
# Begin Source File

SOURCE=.\core\MixtureModel.h
# End Source File
# Begin Source File

SOURCE=.\core\MixtureModelMap.cpp
# End Source File
# Begin Source File

SOURCE=.\core\MixtureModelMap.h
# End Source File
# Begin Source File

SOURCE=.\core\Object.cpp
# End Source File
# Begin Source File

SOURCE=.\core\Object.h
# End Source File
# Begin Source File

SOURCE=.\core\ObjectsInformation.cpp
# End Source File
# Begin Source File

SOURCE=.\core\ObjectsInformation.h
# End Source File
# Begin Source File

SOURCE=.\core\PanoramicDewarper.cpp
# End Source File
# Begin Source File

SOURCE=.\core\PanoramicDewarper.h
# End Source File
# Begin Source File

SOURCE=.\core\PerspectiveDewarper.cpp
# End Source File
# Begin Source File

SOURCE=.\core\PerspectiveDewarper.h
# End Source File
# Begin Source File

SOURCE=.\core\Pipeline.cpp
# End Source File
# Begin Source File

SOURCE=.\core\Pipeline.h
# End Source File
# Begin Source File

SOURCE=.\core\SyncNodeWithStatistics.cpp
# End Source File
# Begin Source File

SOURCE=.\core\SyncNodeWithStatistics.h
# End Source File
# Begin Source File

SOURCE=.\core\Tracker.cpp
# End Source File
# Begin Source File

SOURCE=.\core\Tracker.h
# End Source File
# Begin Source File

SOURCE=.\core\version.cpp
# End Source File
# Begin Source File

SOURCE=.\core\version.h
# End Source File
# Begin Source File

SOURCE=.\core\VirtualCameraController.cpp
# End Source File
# Begin Source File

SOURCE=.\core\VirtualCameraController.h
# End Source File
# End Group
# Begin Group "tools"

# PROP Default_Filter ""
# Begin Group "pview"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\tools\pview\CNTRDATA.cpp
# End Source File
# Begin Source File

SOURCE=.\tools\pview\INSTDATA.cpp
# End Source File
# Begin Source File

SOURCE=.\tools\pview\OBJDATA.cpp
# End Source File
# Begin Source File

SOURCE=.\tools\pview\PERFDATA.cpp
# End Source File
# Begin Source File

SOURCE=.\tools\pview\PERFDATA.H
# End Source File
# Begin Source File

SOURCE=.\tools\pview\pview.cpp
# End Source File
# Begin Source File

SOURCE=.\tools\pview\pview.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\tools\CvUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\tools\CvUtils.h
# End Source File
# Begin Source File

SOURCE=.\tools\DistinctColours.cpp
# End Source File
# Begin Source File

SOURCE=.\tools\DistinctColours.h
# End Source File
# Begin Source File

SOURCE=.\tools\float_cast.h
# End Source File
# Begin Source File

SOURCE=.\tools\hsv.cpp
# End Source File
# Begin Source File

SOURCE=.\tools\hsv.h
# End Source File
# Begin Source File

SOURCE=.\tools\Ijl_Ipl.cpp
# End Source File
# Begin Source File

SOURCE=.\tools\Ijl_Ipl.h
# End Source File
# Begin Source File

SOURCE=.\tools\LogHandler.cpp
# End Source File
# Begin Source File

SOURCE=.\tools\LogHandler.h
# End Source File
# Begin Source File

SOURCE=.\tools\LWGraph.cpp
# End Source File
# Begin Source File

SOURCE=.\tools\LWGraph.h
# End Source File
# Begin Source File

SOURCE=.\tools\LWList.cpp
# End Source File
# Begin Source File

SOURCE=.\tools\LWList.h
# End Source File
# Begin Source File

SOURCE=.\tools\MemInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\tools\MemInfo.h
# End Source File
# Begin Source File

SOURCE=.\tools\Singleton.h
# End Source File
# Begin Source File

SOURCE=.\tools\SingletonDestroyer.h
# End Source File
# Begin Source File

SOURCE=.\tools\ThreadSafeHighgui.cpp
# End Source File
# Begin Source File

SOURCE=.\tools\ThreadSafeHighgui.h
# End Source File
# Begin Source File

SOURCE=.\tools\Timer.h
# End Source File
# Begin Source File

SOURCE=.\tools\trig.cpp
# End Source File
# Begin Source File

SOURCE=.\tools\trig.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\omniapp.ini
# End Source File
# End Target
# End Project
