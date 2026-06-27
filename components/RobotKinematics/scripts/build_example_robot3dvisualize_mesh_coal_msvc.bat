@echo off
REM ============================================================================
REM Build the Robot3DVizualize example linked against the Coal-enabled
REM RobotKinematics library so the in-app collision backend selector can
REM exercise mesh modes. Requires:
REM   - VTK install at VTK_ROOT (same requirement as the default example build)
REM   - Coal/Boost/Assimp roots discoverable for runtime DLL search
REM   - The Coal-enabled library already built via scripts\build_msvc_mesh_coal.bat
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
if "%COAL_ROOT%"=="" set "COAL_ROOT=%ROOT%\third_party\install\coal"
if "%ASSIMP_ROOT%"=="" set "ASSIMP_ROOT=%ROOT%\third_party\install\assimp"
if "%BOOST_ROOT%"=="" set "BOOST_ROOT=%ROOT%\third_party\install\boost"

set "LIB_BUILD=%ROOT%\build\msvc_mesh_coal"
if not exist "%LIB_BUILD%\lib\RobotKinematics.lib" (
    echo [ERROR] Coal-enabled RobotKinematics library not found.
    echo         Run scripts\build_msvc_mesh_coal.bat first.
    exit /b 1
)

set "PROJECT=%ROOT%\examples\Robot3DVizualize\Robot3DVizualize.pro"
set "BUILD=%ROOT%\examples\Robot3DVizualize\build\msvc_mesh_coal"

call "%VCVARS%"
if not defined VCToolsInstallDir ( echo [ERROR] vcvars64 did not initialize the MSVC toolchain & exit /b 1 )

set "PATH=%QT_MSVC_DIR%\bin;%VTK_ROOT%\bin;%COAL_ROOT%\bin;%ASSIMP_ROOT%\bin;%BOOST_ROOT%\lib;%PATH%"

if not exist "%BUILD%" mkdir "%BUILD%"
cd /d "%BUILD%" || exit /b 1

qmake "%PROJECT%" "ROBOTKINEMATICS_LIB_DIR=%LIB_BUILD%\lib" "CONFIG+=robotkinematics_mesh_collision" "MESH_COLLISION_BACKEND=coal" "COAL_ROOT=%COAL_ROOT%" "BOOST_ROOT=%BOOST_ROOT%" "ASSIMP_ROOT=%ASSIMP_ROOT%" || ( echo [ERROR] qmake failed & exit /b 1 )
nmake /nologo || ( echo [ERROR] nmake failed & exit /b 1 )

echo [OK] Robot3DVizualize example (Coal-enabled mesh backend) build complete.
exit /b 0
