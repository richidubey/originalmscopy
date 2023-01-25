<html>
<body>
<pre>
<h1>Build Log</h1>
<h3>
--------------------Configuration: MSCopy - Win32 (WCE THUMB) Release--------------------
</h3>
<h3>Command Lines</h3>
Creating command line "rc.exe /l 0x409 /fo"THUMBRel/MSCopy.res" /d UNDER_CE=300 /d _WIN32_WCE=300 /d "UNICODE" /d "_UNICODE" /d "NDEBUG" /d "UNKNOWN" /d "THUMB" /d "_THUMB_" /d "ARM" /d "_ARM_" /r "D:\Works\Projets\CERN\RAMSES\MSCopy\Code TP277\MSCopy\MSCopy.rc"" 
Creating temporary file "C:\DOCUME~1\assystem\LOCALS~1\Temp\RSP81.tmp" with contents
[
/nologo /W3 /D _WIN32_WCE=300 /D "ARM" /D "_ARM_" /D "UNKNOWN" /D "THUMB" /D "_THUMB_" /D UNDER_CE=300 /D "UNICODE" /D "_UNICODE" /D "NDEBUG" /Fp"THUMBRel/MSCopy.pch" /Yu"stdafx.h" /Fo"THUMBRel/" /QRarch4T /QRinterwork-return /Oxs /MC /c 
"D:\Works\Projets\CERN\RAMSES\MSCopy\Code TP277\MSCopy\CIniFile.cpp"
"D:\Works\Projets\CERN\RAMSES\MSCopy\Code TP277\MSCopy\CLogFileMove.cpp"
"D:\Works\Projets\CERN\RAMSES\MSCopy\Code TP277\MSCopy\CStatusWrite.cpp"
"D:\Works\Projets\CERN\RAMSES\MSCopy\Code TP277\MSCopy\CUsrSynchro.cpp"
"D:\Works\Projets\CERN\RAMSES\MSCopy\Code TP277\MSCopy\MSCopy.cpp"
]
Creating command line "clthumb.exe @C:\DOCUME~1\assystem\LOCALS~1\Temp\RSP81.tmp" 
Creating temporary file "C:\DOCUME~1\assystem\LOCALS~1\Temp\RSP82.tmp" with contents
[
/nologo /W3 /D _WIN32_WCE=300 /D "ARM" /D "_ARM_" /D "UNKNOWN" /D "THUMB" /D "_THUMB_" /D UNDER_CE=300 /D "UNICODE" /D "_UNICODE" /D "NDEBUG" /Fp"THUMBRel/MSCopy.pch" /Yc"stdafx.h" /Fo"THUMBRel/" /QRarch4T /QRinterwork-return /Oxs /MC /c 
"D:\Works\Projets\CERN\RAMSES\MSCopy\Code TP277\MSCopy\StdAfx.cpp"
]
Creating command line "clthumb.exe @C:\DOCUME~1\assystem\LOCALS~1\Temp\RSP82.tmp" 
Creating temporary file "C:\DOCUME~1\assystem\LOCALS~1\Temp\RSP83.tmp" with contents
[
commctrl.lib coredll.lib toolhelp.lib /nologo /base:"0x00010000" /stack:0x10000,0x1000 /entry:"WinMainCRTStartup" /incremental:no /pdb:"THUMBRel/MSCopy.pdb" /nodefaultlib:"libc.lib /nodefaultlib:libcd.lib /nodefaultlib:libcmt.lib /nodefaultlib:libcmtd.lib /nodefaultlib:msvcrt.lib /nodefaultlib:msvcrtd.lib" /out:"THUMBRel/MSCopy.exe" /subsystem:windowsce,3.00 /MACHINE:THUMB 
".\THUMBRel\CIniFile.obj"
".\THUMBRel\CLogFileMove.obj"
".\THUMBRel\CStatusWrite.obj"
".\THUMBRel\CUsrSynchro.obj"
".\THUMBRel\MSCopy.obj"
".\THUMBRel\StdAfx.obj"
".\THUMBRel\MSCopy.res"
]
Creating command line "link.exe @C:\DOCUME~1\assystem\LOCALS~1\Temp\RSP83.tmp"
<h3>Output Window</h3>
Compiling resources...
Compiling...
StdAfx.cpp
Compiling...
CIniFile.cpp
CLogFileMove.cpp
CStatusWrite.cpp
CUsrSynchro.cpp
MSCopy.cpp
Generating Code...
Linking...




<h3>Results</h3>
MSCopy.exe - 0 error(s), 0 warning(s)
</pre>
</body>
</html>
