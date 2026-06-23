@echo off
REM ============================================================================
REM Build the Robot3DVizualize Qt/VTK example with MSVC.
REM
REM VTK is an external dependency for this example only. The core library build
REM does not require VTK.
REM
REM Required:
REM   VTK_ROOT      VTK install prefix, defaults to D:\Project\vtk_build\vtk\install-x64-cuda-qt-vs when present
REM Optional:
REM   VTK_VERSION   default 9.6
REM   QT_MSVC_DIR   default C:\Qt\6.8.3\msvc2022_64
REM   VCVARS        default VS 2022 Community vcvars64.bat
REM ============================================================================
setlocal

if "%QT_MSVC_DIR%"=="" set "QT_MSVC_DIR=C:\Qt\6.8.3\msvc2022_64"
if "%VCVARS%"=="" set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if "%VTK_VERSION%"=="" set "VTK_VERSION=9.6"

if "%VTK_ROOT%"=="" (
    if exist "D:\Project\vtk_build\vtk\install-x64-cuda-qt-vs" (
        set "VTK_ROOT=D:\Project\vtk_build\vtk\install-x64-cuda-qt-vs"
    ) else (
        echo [ERROR] VTK_ROOT is not set.
        echo         Install VTK externally, then set VTK_ROOT to its install prefix.
        echo         Example: set VTK_ROOT=C:\VTK\install
        exit /b 1
    )
)

set "ROOT=%~dp0.."
set "PROJECT=%ROOT%\examples\Robot3DVizualize\Robot3DVizualize.pro"
set "BUILD=%ROOT%\examples\Robot3DVizualize\build\msvc"

call "%ROOT%\scripts\build_msvc.bat"
if errorlevel 1 (
    echo [ERROR] Core RobotKinematics build failed.
    exit /b 1
)

call "%VCVARS%"
if not defined VCToolsInstallDir ( echo [ERROR] vcvars64 did not initialize the MSVC toolchain & exit /b 1 )
if not defined INCLUDE ( echo [ERROR] INCLUDE not set - vcvars64 setup incomplete & exit /b 1 )

set "PATH=%QT_MSVC_DIR%\bin;%VTK_ROOT%\bin;%PATH%"

if not exist "%BUILD%" mkdir "%BUILD%"
cd /d "%BUILD%" || exit /b 1

qmake "%PROJECT%" || ( echo [ERROR] qmake failed & exit /b 1 )
nmake /nologo || ( echo [ERROR] nmake failed & exit /b 1 )

echo [OK] Robot3DVizualize example build complete.
exit /b 0
