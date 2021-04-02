# Microsoft Developer Studio Project File - Name="EasyDRF" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=EasyDRF - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "EasyDRF.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "EasyDRF.mak" CFG="EasyDRF - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "EasyDRF - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "EasyDRF - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "EasyDRF - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# SUBTRACT CPP /O<none>
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x807 /d "NDEBUG"
# ADD RSC /l 0x807 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comdlg32.lib shell32.lib winmm.lib libspeex.lib libfftw.lib mixer.lib ptt.lib graphwin.lib lpc_10.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"libc.lib" /libpath:"common/libs/"

!ELSEIF  "$(CFG)" == "EasyDRF - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x807 /d "_DEBUG"
# ADD RSC /l 0x807 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib libfftw.lib libspeex.lib ptt.lib mixer.lib lpc_10.lib graphwin.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"libc.lib" /pdbtype:sept /libpath:"common/libs/"

!ENDIF 

# Begin Target

# Name "EasyDRF - Win32 Release"
# Name "EasyDRF - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "common"

# PROP Default_Filter ""
# Begin Group "chanest"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\common\chanest\ChanEstTime.cpp
# End Source File
# Begin Source File

SOURCE=.\common\chanest\ChanEstTime.h
# End Source File
# Begin Source File

SOURCE=.\common\chanest\ChannelEstimation.cpp
# End Source File
# Begin Source File

SOURCE=.\common\chanest\ChannelEstimation.h
# End Source File
# Begin Source File

SOURCE=.\common\chanest\IdealChannelEstimation.cpp
# End Source File
# Begin Source File

SOURCE=.\common\chanest\IdealChannelEstimation.h
# End Source File
# Begin Source File

SOURCE=.\common\chanest\TimeLinear.cpp
# End Source File
# Begin Source File

SOURCE=.\common\chanest\TimeLinear.h
# End Source File
# Begin Source File

SOURCE=.\common\chanest\TimeWiener.cpp
# End Source File
# Begin Source File

SOURCE=.\common\chanest\TimeWiener.h
# End Source File
# End Group
# Begin Group "datadecoding"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\common\datadecoding\DABMOT.cpp
# End Source File
# Begin Source File

SOURCE=.\common\datadecoding\DABMOT.h
# End Source File
# Begin Source File

SOURCE=.\common\datadecoding\DataDecoder.cpp
# End Source File
# Begin Source File

SOURCE=.\common\datadecoding\DataDecoder.h
# End Source File
# Begin Source File

SOURCE=.\common\datadecoding\Journaline.h
# End Source File
# Begin Source File

SOURCE=.\common\datadecoding\MOTSlideShow.cpp
# End Source File
# Begin Source File

SOURCE=.\common\datadecoding\MOTSlideShow.h
# End Source File
# Begin Source File

SOURCE=.\common\datadecoding\picpool.cpp
# End Source File
# Begin Source File

SOURCE=.\common\datadecoding\picpool.h
# End Source File
# End Group
# Begin Group "fac"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\common\FAC\FAC.cpp
# End Source File
# Begin Source File

SOURCE=.\common\FAC\FAC.h
# End Source File
# End Group
# Begin Group "interleaver"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\common\interleaver\BlockInterleaver.cpp
# End Source File
# Begin Source File

SOURCE=.\common\interleaver\BlockInterleaver.h
# End Source File
# Begin Source File

SOURCE=.\common\interleaver\SymbolInterleaver.cpp
# End Source File
# Begin Source File

SOURCE=.\common\interleaver\SymbolInterleaver.h
# End Source File
# End Group
# Begin Group "lpc_10"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\common\lpc_10\LPC10.H
# End Source File
# End Group
# Begin Group "matlib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\common\matlib\Matlib.h
# End Source File
# Begin Source File

SOURCE=.\common\matlib\MatlibSigProToolbox.cpp
# End Source File
# Begin Source File

SOURCE=.\common\matlib\MatlibSigProToolbox.h
# End Source File
# Begin Source File

SOURCE=.\common\matlib\MatlibStdToolbox.cpp
# End Source File
# Begin Source File

SOURCE=.\common\matlib\MatlibStdToolbox.h
# End Source File
# End Group
# Begin Group "mlc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\common\mlc\BitInterleaver.cpp
# End Source File
# Begin Source File

SOURCE=.\common\mlc\BitInterleaver.h
# End Source File
# Begin Source File

SOURCE=.\common\mlc\ChannelCode.cpp
# End Source File
# Begin Source File

SOURCE=.\common\mlc\ChannelCode.h
# End Source File
# Begin Source File

SOURCE=.\common\mlc\ConvEncoder.cpp
# End Source File
# Begin Source File

SOURCE=.\common\mlc\ConvEncoder.h
# End Source File
# Begin Source File

SOURCE=.\common\mlc\EnergyDispersal.cpp
# End Source File
# Begin Source File

SOURCE=.\common\mlc\EnergyDispersal.h
# End Source File
# Begin Source File

SOURCE=.\common\mlc\Metric.cpp
# End Source File
# Begin Source File

SOURCE=.\common\mlc\Metric.h
# End Source File
# Begin Source File

SOURCE=.\common\mlc\MLC.cpp
# End Source File
# Begin Source File

SOURCE=.\common\mlc\MLC.h
# End Source File
# Begin Source File

SOURCE=.\common\mlc\QAMMapping.cpp
# End Source File
# Begin Source File

SOURCE=.\common\mlc\QAMMapping.h
# End Source File
# Begin Source File

SOURCE=.\common\mlc\TrellisUpdateMMX.cpp
# End Source File
# Begin Source File

SOURCE=.\common\mlc\TrellisUpdateSSE2.cpp
# End Source File
# Begin Source File

SOURCE=.\common\mlc\ViterbiDecoder.cpp
# End Source File
# Begin Source File

SOURCE=.\common\mlc\ViterbiDecoder.h
# End Source File
# End Group
# Begin Group "odfmcellmapping"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\common\ofdmcellmapping\CellMappingTable.cpp
# End Source File
# Begin Source File

SOURCE=.\common\ofdmcellmapping\CellMappingTable.h
# End Source File
# Begin Source File

SOURCE=.\common\ofdmcellmapping\OFDMCellMapping.cpp
# End Source File
# Begin Source File

SOURCE=.\common\ofdmcellmapping\OFDMCellMapping.h
# End Source File
# End Group
# Begin Group "resample"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\common\resample\Resample.cpp
# End Source File
# Begin Source File

SOURCE=.\common\resample\Resample.h
# End Source File
# Begin Source File

SOURCE=.\common\resample\ResampleFilter.h
# End Source File
# End Group
# Begin Group "sourcedecoders"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\common\sourcedecoders\AudioSourceDecoder.cpp
# End Source File
# Begin Source File

SOURCE=.\common\sourcedecoders\AudioSourceDecoder.h
# End Source File
# End Group
# Begin Group "speex"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\common\speex\speex.h
# End Source File
# Begin Source File

SOURCE=.\common\speex\speex_bits.h
# End Source File
# Begin Source File

SOURCE=.\common\speex\speex_callbacks.h
# End Source File
# Begin Source File

SOURCE=.\common\speex\speex_header.h
# End Source File
# Begin Source File

SOURCE=.\common\speex\speex_stereo.h
# End Source File
# End Group
# Begin Group "sync"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\common\sync\FreqSyncAcq.cpp
# End Source File
# Begin Source File

SOURCE=.\common\sync\FreqSyncAcq.h
# End Source File
# Begin Source File

SOURCE=.\common\sync\SyncUsingPil.cpp
# End Source File
# Begin Source File

SOURCE=.\common\sync\SyncUsingPil.h
# End Source File
# Begin Source File

SOURCE=.\common\sync\TimeSync.cpp
# End Source File
# Begin Source File

SOURCE=.\common\sync\TimeSync.h
# End Source File
# Begin Source File

SOURCE=.\common\sync\TimeSyncFilter.h
# End Source File
# Begin Source File

SOURCE=.\common\sync\TimeSyncTrack.cpp
# End Source File
# Begin Source File

SOURCE=.\common\sync\TimeSyncTrack.h
# End Source File
# End Group
# Begin Group "tables"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\common\tables\TableCarMap.h
# End Source File
# Begin Source File

SOURCE=.\common\tables\TableCarrier.h
# End Source File
# Begin Source File

SOURCE=.\common\tables\TableDRMGlobal.h
# End Source File
# Begin Source File

SOURCE=.\common\tables\TableFAC.h
# End Source File
# Begin Source File

SOURCE=.\common\tables\TableMLC.h
# End Source File
# Begin Source File

SOURCE=.\common\tables\TableQAMMapping.h
# End Source File
# End Group
# Begin Group "libs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\common\libs\graphwin.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\common\AudioFile.h
# End Source File
# Begin Source File

SOURCE=.\common\audiofir.cpp
# End Source File
# Begin Source File

SOURCE=.\common\audiofir.h
# End Source File
# Begin Source File

SOURCE=.\common\bsr.cpp
# End Source File
# Begin Source File

SOURCE=.\common\bsr.h
# End Source File
# Begin Source File

SOURCE=.\common\Buffer.h
# End Source File
# Begin Source File

SOURCE=.\common\CRC.cpp
# End Source File
# Begin Source File

SOURCE=.\common\CRC.h
# End Source File
# Begin Source File

SOURCE=.\common\Data.cpp
# End Source File
# Begin Source File

SOURCE=.\common\Data.h
# End Source File
# Begin Source File

SOURCE=.\common\DrmReceiver.cpp
# End Source File
# Begin Source File

SOURCE=.\common\DrmReceiver.h
# End Source File
# Begin Source File

SOURCE=.\common\DRMSignalIO.cpp
# End Source File
# Begin Source File

SOURCE=.\common\DRMSignalIO.h
# End Source File
# Begin Source File

SOURCE=.\common\DrmTransmitter.cpp
# End Source File
# Begin Source File

SOURCE=.\common\DrmTransmitter.h
# End Source File
# Begin Source File

SOURCE=.\common\fir.cpp
# End Source File
# Begin Source File

SOURCE=.\common\fir.h
# End Source File
# Begin Source File

SOURCE=.\common\GlobalDefinitions.h
# End Source File
# Begin Source File

SOURCE=.\common\InputResample.cpp
# End Source File
# Begin Source File

SOURCE=.\common\InputResample.h
# End Source File
# Begin Source File

SOURCE=.\common\list.cpp
# End Source File
# Begin Source File

SOURCE=.\common\list.h
# End Source File
# Begin Source File

SOURCE=.\common\Modul.h
# End Source File
# Begin Source File

SOURCE=.\common\MSCMultiplexer.cpp
# End Source File
# Begin Source File

SOURCE=.\common\MSCMultiplexer.h
# End Source File
# Begin Source File

SOURCE=.\common\OFDM.cpp
# End Source File
# Begin Source File

SOURCE=.\common\OFDM.h
# End Source File
# Begin Source File

SOURCE=.\common\Parameter.cpp
# End Source File
# Begin Source File

SOURCE=.\common\Parameter.h
# End Source File
# Begin Source File

SOURCE=.\common\settings.cpp
# End Source File
# Begin Source File

SOURCE=.\common\settings.h
# End Source File
# Begin Source File

SOURCE=.\common\TextMessage.cpp
# End Source File
# Begin Source File

SOURCE=.\common\TextMessage.h
# End Source File
# Begin Source File

SOURCE=.\common\TransmitterFilter.h
# End Source File
# Begin Source File

SOURCE=.\common\Vector.h
# End Source File
# End Group
# Begin Group "sound"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\sound\Sound.cpp
# End Source File
# Begin Source File

SOURCE=.\sound\Sound.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\Dialog.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialog.h
# End Source File
# Begin Source File

SOURCE=.\getfilenam.cpp
# End Source File
# Begin Source File

SOURCE=.\getfilenam.h
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\main.h
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\common\libs\callsign.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\EasyDRF.ico
# End Source File
# Begin Source File

SOURCE=.\EasyDRF.rc
# End Source File
# End Group
# End Target
# End Project
