@echo off
setlocal

if "%QT_MSVC_DIR%"=="" ( echo [ERROR] QT_MSVC_DIR must point to the Qt MSVC kit root & exit /b 1 )

set "ROOT=%~dp0.."
if "%COAL_ROOT%"=="" set "COAL_ROOT=%ROOT%\third_party\install\coal"
if "%ASSIMP_ROOT%"=="" set "ASSIMP_ROOT=%ROOT%\third_party\install\assimp"
if "%BOOST_ROOT%"=="" set "BOOST_ROOT=%ROOT%\third_party\install\boost"
set "EXE=%ROOT%\build\tools\mesh_collision_benchmark\release\mesh_collision_benchmark.exe"

if not exist "%EXE%" (
    echo [ERROR] %EXE% not found. Run scripts\build_mesh_collision_benchmark_msvc.bat first.
    exit /b 1
)

set "PATH=%QT_MSVC_DIR%\bin;%COAL_ROOT%\bin;%ASSIMP_ROOT%\bin;%BOOST_ROOT%\lib;%PATH%"
"%EXE%" %* || ( echo [ERROR] mesh_collision_benchmark.exe failed & exit /b 1 )

echo [OK] MSVC mesh collision benchmark run complete.
exit /b 0
