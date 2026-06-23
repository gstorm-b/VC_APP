@echo off
setlocal

if "%QT_MSVC_DIR%"=="" set "QT_MSVC_DIR=C:\Qt\6.8.3\msvc2022_64"
if "%VCVARS%"=="" set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

set "ROOT=%~dp0.."
if "%COAL_ROOT%"=="" set "COAL_ROOT=%ROOT%\third_party\install\coal"
if "%ASSIMP_ROOT%"=="" set "ASSIMP_ROOT=%ROOT%\third_party\install\assimp"
if "%BOOST_ROOT%"=="" set "BOOST_ROOT=%ROOT%\third_party\install\boost"
set "BUILD=%ROOT%\build\msvc_mesh_coal"

call "%VCVARS%"
if not defined VCToolsInstallDir ( echo [ERROR] vcvars64 did not initialize the MSVC toolchain & exit /b 1 )
if not defined INCLUDE ( echo [ERROR] INCLUDE not set - vcvars64 setup incomplete & exit /b 1 )
set "PATH=%QT_MSVC_DIR%\bin;%COAL_ROOT%\bin;%ASSIMP_ROOT%\bin;%BOOST_ROOT%\lib;%PATH%"

if not exist "%BUILD%" mkdir "%BUILD%"
cd /d "%BUILD%" || exit /b 1

qmake "%ROOT%\RobotKinematics.pro" "CONFIG+=robotkinematics_mesh_collision" "MESH_COLLISION_BACKEND=coal" "COAL_ROOT=%COAL_ROOT%" "BOOST_ROOT=%BOOST_ROOT%" "ASSIMP_ROOT=%ASSIMP_ROOT%" || ( echo [ERROR] qmake failed & exit /b 1 )
nmake /nologo || ( echo [ERROR] nmake failed & exit /b 1 )

echo [OK] MSVC Coal mesh build complete.
exit /b 0
