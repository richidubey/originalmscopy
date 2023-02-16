<html>
<body>
<pre>
<h1>Build Log</h1>
<h3>
--------------------Configuration: MSCopyThumb - Win32 (WCE THUMB) Debug--------------------
</h3>
<h3>Command Lines</h3>
Creating command line "rc.exe /l 0x409 /fo"THUMBDbg/MSCopyThumb.res" /d UNDER_CE=300 /d _WIN32_WCE=300 /d "UNICODE" /d "_UNICODE" /d "DEBUG" /d "UNKNOWN" /d "THUMB" /d "_THUMB_" /d "ARM" /d "_ARM_" /r "C:\repos\mscopy\MSCopyThumb.rc"" 
Creating temporary file "C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp\RSP1D5.tmp" with contents
[
/nologo /W3 /Zi /Od /D "DEBUG" /D "THUMB" /D "_THUMB_" /D UNDER_CE=300 /D _WIN32_WCE=300 /D "ARM" /D "_ARM_" /D "UNKNOWN" /D "UNICODE" /D "_UNICODE" /Fp"THUMBDbg/MSCopyThumb.pch" /Yu"stdafx.h" /Fo"THUMBDbg/" /Fd"THUMBDbg/" /QRarch4T /QRinterwork-return /MC /c 
"C:\repos\MSCopy\CIniFile.cpp"
"C:\repos\MSCopy\CLogFileMove.cpp"
"C:\repos\MSCopy\CStatusWrite.cpp"
"C:\repos\MSCopy\CUsrSynchro.cpp"
"C:\repos\mscopy\MSCopyThumb.cpp"
]
Creating command line "clthumb.exe @C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp\RSP1D5.tmp" 
Creating temporary file "C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp\RSP1D6.tmp" with contents
[
/nologo /W3 /Zi /Od /D "DEBUG" /D "THUMB" /D "_THUMB_" /D UNDER_CE=300 /D _WIN32_WCE=300 /D "ARM" /D "_ARM_" /D "UNKNOWN" /D "UNICODE" /D "_UNICODE" /Fp"THUMBDbg/MSCopyThumb.pch" /Yc"stdafx.h" /Fo"THUMBDbg/" /Fd"THUMBDbg/" /QRarch4T /QRinterwork-return /MC /c 
"C:\repos\mscopy\StdAfx.cpp"
]
Creating command line "clthumb.exe @C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp\RSP1D6.tmp" 
Creating temporary file "C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp\RSP1D7.tmp" with contents
[
commctrl.lib coredll.lib toolhelp.lib winsock.lib /nologo /base:"0x00010000" /stack:0x10000,0x1000 /entry:"WinMainCRTStartup" /incremental:yes /pdb:"THUMBDbg/MSCopyThumb.pdb" /debug /nodefaultlib:"libc.lib /nodefaultlib:libcd.lib /nodefaultlib:libcmt.lib /nodefaultlib:libcmtd.lib /nodefaultlib:msvcrt.lib /nodefaultlib:msvcrtd.lib" /out:"THUMBDbg/MSCopyThumb.exe" /subsystem:windowsce,3.00 /MACHINE:THUMB 
.\THUMBDbg\CIniFile.obj
.\THUMBDbg\CLogFileMove.obj
.\THUMBDbg\CStatusWrite.obj
.\THUMBDbg\CUsrSynchro.obj
.\THUMBDbg\MSCopyThumb.obj
.\THUMBDbg\StdAfx.obj
.\THUMBDbg\MSCopyThumb.res
]
Creating command line "link.exe @C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp\RSP1D7.tmp"
<h3>Output Window</h3>
Compiling resources...
Compiling...
StdAfx.cpp
Compiling...
CIniFile.cpp
CLogFileMove.cpp
CStatusWrite.cpp
CUsrSynchro.cpp
MSCopyThumb.cpp
Generating Code...
Linking...



<h3>Results</h3>
MSCopyThumb.exe - 0 error(s), 0 warning(s)
</pre>
</body>
</html>
