@echo off
REM ============================================================================
REM Incremental OUT-OF-SOURCE MSVC build for RobotKinematics.
REM
REM Handles the build traps:
REM   - Eigen is vendored at third_party/eigen (no dependency on a system Eigen).
REM   - Builds out-of-source so in-source artifacts never contaminate it.
REM   - Does NOT redirect vcvars64 output (redirecting breaks its SDK setup).
REM
REM This script builds only. Use test_msvc.bat to run tests, or rebuild_msvc.bat
REM for a clean rebuild + test cycle.
REM
REM Override these via environment if your install differs:
REM   QT_MSVC_DIR  (default C:\Qt\6.8.3\msvc2022_64)
REM   VCVARS       (default VS 2022 Community vcvars64.bat)
REM ============================================================================
setlocal
if "%QT_MSVC_DIR%"=="" set "QT_MSVC_DIR=C:\Qt\6.8.3\msvc2022_64"
if "%VCVARS%"=="" set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
set "ROOT=%~dp0.."
set "BUILD=%ROOT%\build\msvc"

call "%VCVARS%"
if not defined VCToolsInstallDir ( echo [ERROR] vcvars64 did not initialize the MSVC toolchain & exit /b 1 )
if not defined INCLUDE ( echo [ERROR] INCLUDE not set - vcvars64 setup incomplete & exit /b 1 )
set "PATH=%QT_MSVC_DIR%\bin;%PATH%"

if not exist "%BUILD%" mkdir "%BUILD%"
cd /d "%BUILD%" || exit /b 1

qmake "%ROOT%\RobotKinematics.pro" || ( echo [ERROR] qmake failed & exit /b 1 )
nmake /nologo || ( echo [ERROR] nmake failed & exit /b 1 )

echo [OK] MSVC build complete.
exit /b 0
