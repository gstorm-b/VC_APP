@echo off
REM ============================================================================
REM Build the VTK debug mesh-collision spike with MSVC.
REM
REM This tool is a Phase 10.1 dependency probe only. It is not part of the core
REM RobotKinematics library build and does not imply VTK is approved for the
REM production mesh backend.
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
    if exist "C:\Program Files\VTK" (
        set "VTK_ROOT=C:\Program Files\VTK"
    ) else (
        echo [ERROR] VTK_ROOT is not set.
        echo         Install VTK externally, then set VTK_ROOT to its install prefix.
        exit /b 1
    )
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
