@echo off
setlocal

if "%QT_MSVC_DIR%"=="" ( echo [ERROR] QT_MSVC_DIR must point to the Qt MSVC kit root & exit /b 1 )

set "ROOT=%~dp0.."
if "%COAL_ROOT%"=="" set "COAL_ROOT=%ROOT%\third_party\install\coal"
if "%ASSIMP_ROOT%"=="" set "ASSIMP_ROOT=%ROOT%\third_party\install\assimp"
if "%BOOST_ROOT%"=="" set "BOOST_ROOT=%ROOT%\third_party\install\boost"
set "BUILD=%ROOT%\build\msvc_mesh_coal"
set "EXE=%BUILD%\tests\RobotKinematicsTests.exe"

if not exist "%EXE%" (
    echo [ERROR] %EXE% not found. Run scripts\build_msvc_mesh_coal.bat first.
    exit /b 1
)

set "PATH=%QT_MSVC_DIR%\bin;%COAL_ROOT%\bin;%ASSIMP_ROOT%\bin;%BOOST_ROOT%\lib;%PATH%"
"%EXE%" || ( echo [ERROR] RobotKinematicsTests.exe failed & exit /b 1 )

echo [OK] MSVC Coal mesh tests complete.
exit /b 0
