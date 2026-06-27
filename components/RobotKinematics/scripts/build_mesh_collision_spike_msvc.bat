@echo off
REM ============================================================================
REM Build the VTK debug mesh-collision spike with MSVC.
REM
REM This tool is a Phase 10.1 dependency probe only. It is not part of the core
REM RobotKinematics library build and does not imply VTK is approved for the
REM production mesh backend.
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
set "PROJECT=%ROOT%\tools\mesh_collision_spike\mesh_collision_spike.pro"
set "BUILD=%ROOT%\build\tools\mesh_collision_spike"

call "%VCVARS%"
if not defined VCToolsInstallDir ( echo [ERROR] vcvars64 did not initialize the MSVC toolchain & exit /b 1 )
if not defined INCLUDE ( echo [ERROR] INCLUDE not set - vcvars64 setup incomplete & exit /b 1 )

set "PATH=%QT_MSVC_DIR%\bin;%VTK_ROOT%\bin;%PATH%"

if not exist "%BUILD%" mkdir "%BUILD%"
cd /d "%BUILD%" || exit /b 1

qmake "%PROJECT%" || ( echo [ERROR] qmake failed & exit /b 1 )
nmake /nologo || ( echo [ERROR] nmake failed & exit /b 1 )

echo [OK] mesh_collision_spike build complete.
exit /b 0
