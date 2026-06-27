@echo off
REM ============================================================================
REM Build the Robot3DVizualize Qt/VTK example with MSVC.
REM
REM VTK is an external dependency for this example only. The core library build
REM does not require VTK.
REM
REM Required:
REM   VTK_ROOT      VTK install prefix
REM Optional:
REM   VTK_VERSION   default 9.6
REM   QT_MSVC_DIR   Qt MSVC kit root
REM   VCVARS        full path to vcvars64.bat
REM ============================================================================
setlocal

if "%QT_MSVC_DIR%"=="" ( echo [ERROR] QT_MSVC_DIR must point to the Qt MSVC kit root & exit /b 1 )
if "%VCVARS%"=="" ( echo [ERROR] VCVARS must point to vcvars64.bat & exit /b 1 )
if "%VTK_VERSION%"=="" set "VTK_VERSION=9.6"

if "%VTK_ROOT%"=="" (
    echo [ERROR] VTK_ROOT must point to the VTK install prefix.
    exit /b 1
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
