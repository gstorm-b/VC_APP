@echo off
setlocal

if "%QT_MSVC_DIR%"=="" ( echo [ERROR] QT_MSVC_DIR must point to the Qt MSVC kit root & exit /b 1 )
if "%VCVARS%"=="" ( echo [ERROR] VCVARS must point to vcvars64.bat & exit /b 1 )

set "ROOT=%~dp0.."
if "%COAL_ROOT%"=="" set "COAL_ROOT=%ROOT%\third_party\install\coal"
if "%ASSIMP_ROOT%"=="" set "ASSIMP_ROOT=%ROOT%\third_party\install\assimp"
if "%BOOST_ROOT%"=="" set "BOOST_ROOT=%ROOT%\third_party\install\boost"
set "ROBOTKINEMATICS_LIB_DIR=%ROOT%\build\msvc_mesh_coal\lib"
set "BUILD=%ROOT%\build\tools\mesh_collision_benchmark"

call "%VCVARS%"
if not defined VCToolsInstallDir ( echo [ERROR] vcvars64 did not initialize the MSVC toolchain & exit /b 1 )
if not defined INCLUDE ( echo [ERROR] INCLUDE not set - vcvars64 setup incomplete & exit /b 1 )
set "PATH=%QT_MSVC_DIR%\bin;%COAL_ROOT%\bin;%ASSIMP_ROOT%\bin;%BOOST_ROOT%\lib;%PATH%"

call "%ROOT%\scripts\build_msvc_mesh_coal.bat" || exit /b 1

if not exist "%BUILD%" mkdir "%BUILD%"
cd /d "%BUILD%" || exit /b 1

qmake "%ROOT%\tools\mesh_collision_benchmark\mesh_collision_benchmark.pro" ^
    "CONFIG+=robotkinematics_mesh_collision" ^
    "MESH_COLLISION_BACKEND=coal" ^
    "COAL_ROOT=%COAL_ROOT%" ^
    "BOOST_ROOT=%BOOST_ROOT%" ^
    "ASSIMP_ROOT=%ASSIMP_ROOT%" ^
    "ROBOTKINEMATICS_LIB_DIR=%ROBOTKINEMATICS_LIB_DIR%" || ( echo [ERROR] qmake failed & exit /b 1 )
nmake /nologo || ( echo [ERROR] nmake failed & exit /b 1 )

echo [OK] MSVC mesh collision benchmark build complete.
exit /b 0
