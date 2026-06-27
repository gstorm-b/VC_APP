@echo off
REM ============================================================================
REM Build the mesh simplification tool against the default (no mesh backend)
REM RobotKinematics library build. The tool itself only depends on Qt + Eigen
REM + the library, so it does not need the optional Coal/FCL stack.
REM ============================================================================
setlocal

if "%QT_MSVC_DIR%"=="" ( echo [ERROR] QT_MSVC_DIR must point to the Qt MSVC kit root & exit /b 1 )
if "%VCVARS%"=="" ( echo [ERROR] VCVARS must point to vcvars64.bat & exit /b 1 )

set "ROOT=%~dp0.."
set "LIB_BUILD=%ROOT%\build\msvc"
set "BUILD=%ROOT%\build\tools\mesh_simplification"

if not exist "%LIB_BUILD%\lib\RobotKinematics.lib" (
    echo [ERROR] RobotKinematics library not built. Run scripts\build_msvc.bat first.
    exit /b 1
)

call "%VCVARS%"
if not defined VCToolsInstallDir ( echo [ERROR] vcvars64 did not initialize the MSVC toolchain & exit /b 1 )
if not defined INCLUDE ( echo [ERROR] INCLUDE not set - vcvars64 setup incomplete & exit /b 1 )
set "PATH=%QT_MSVC_DIR%\bin;%PATH%"

if not exist "%BUILD%" mkdir "%BUILD%"
cd /d "%BUILD%" || exit /b 1

set "ROBOTKINEMATICS_LIB_DIR=%LIB_BUILD%\lib"

qmake "%ROOT%\tools\mesh_simplification\mesh_simplification.pro" "ROBOTKINEMATICS_LIB_DIR=%ROBOTKINEMATICS_LIB_DIR%" || ( echo [ERROR] qmake failed & exit /b 1 )
nmake /nologo || ( echo [ERROR] nmake failed & exit /b 1 )

echo [OK] MSVC mesh simplification tool build complete.
exit /b 0
