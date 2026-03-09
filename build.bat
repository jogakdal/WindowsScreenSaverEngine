@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

set VCDIR=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207
set WINSDK=C:\Program Files (x86)\Windows Kits\10
set WINSDKVER=10.0.26100.0

set PATH=%VCDIR%\bin\Hostx64\x64;%WINSDK%\bin\%WINSDKVER%\x64;%PATH%
set INCLUDE=%VCDIR%\include;%WINSDK%\Include\%WINSDKVER%\ucrt;%WINSDK%\Include\%WINSDKVER%\um;%WINSDK%\Include\%WINSDKVER%\shared
set LIB=%VCDIR%\lib\x64;%WINSDK%\Lib\%WINSDKVER%\ucrt\x64;%WINSDK%\Lib\%WINSDKVER%\um\x64

set SRCDIR=C:\Users\jogak\project\ScreenSaver\WindowsScreenSaverEngine\src
set OUTDIR=C:\Users\jogak\project\ScreenSaver\WindowsScreenSaverEngine\lib\Release
set OBJDIR=C:\Users\jogak\project\ScreenSaver\WindowsScreenSaverEngine\obj\Release

if not exist "%OUTDIR%" mkdir "%OUTDIR%"
if not exist "%OBJDIR%" mkdir "%OBJDIR%"
if not exist "%OBJDIR%\framework" mkdir "%OBJDIR%\framework"
if not exist "%OBJDIR%\overlay" mkdir "%OBJDIR%\overlay"
if not exist "%OBJDIR%\render" mkdir "%OBJDIR%\render"

set CFLAGS=/nologo /c /O2 /GL /MT /W4 /EHsc /std:c++20 /utf-8 /fp:fast /DUNICODE /D_UNICODE /DNOMINMAX /I"%SRCDIR%"
set LFLAGS=/nologo /LTCG

echo [1/3] Compiling framework sources...

set SOURCES_FW=Window ScreenSaverWindow PreviewWindow ConfigDialog Registry UpdateChecker Localization ScreenSaverEngine
set SOURCES_OV=SystemInfo TextOverlay ClockOverlay
set SOURCES_RN=RenderSurface SegmentDisplay

for %%f in (%SOURCES_FW%) do (
    echo   framework\%%f.cpp
    cl %CFLAGS% /Fo"%OBJDIR%\framework\%%f.obj" "%SRCDIR%\framework\%%f.cpp"
    if errorlevel 1 goto :error
)

for %%f in (%SOURCES_OV%) do (
    echo   overlay\%%f.cpp
    cl %CFLAGS% /Fo"%OBJDIR%\overlay\%%f.obj" "%SRCDIR%\overlay\%%f.cpp"
    if errorlevel 1 goto :error
)

for %%f in (%SOURCES_RN%) do (
    echo   render\%%f.cpp
    cl %CFLAGS% /Fo"%OBJDIR%\render\%%f.obj" "%SRCDIR%\render\%%f.cpp"
    if errorlevel 1 goto :error
)

echo [2/3] Creating static library...
set OBJS=
for %%f in (%SOURCES_FW%) do set OBJS=!OBJS! "%OBJDIR%\framework\%%f.obj"
for %%f in (%SOURCES_OV%) do set OBJS=!OBJS! "%OBJDIR%\overlay\%%f.obj"
for %%f in (%SOURCES_RN%) do set OBJS=!OBJS! "%OBJDIR%\render\%%f.obj"

lib %LFLAGS% /OUT:"%OUTDIR%\wsse.lib" %OBJS%
if errorlevel 1 goto :error

echo [3/3] Build successful!
echo Output: %OUTDIR%\wsse.lib
goto :end

:error
echo BUILD FAILED!
exit /b 1

:end
echo Done.
