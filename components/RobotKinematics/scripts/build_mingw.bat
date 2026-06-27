@echo off
REM ============================================================================
REM Incremental OUT-OF-SOURCE MinGW build for RobotKinematics.
REM
REM Handles the build traps:
REM   - Eigen is vendored at third_party/eigen (no dependency on a system Eigen,
REM     e.g. MSYS2's). The build is self-contained.
REM   - Builds out-of-source so in-source artifacts never contaminate it.
REM
REM This script builds only. Use test_mingw.bat to run tests, or rebuild_mingw.bat
REM for a clean rebuild + test cycle.
REM
REM Override these via environment if your install differs:
REM   QT_MINGW_DIR  Qt MinGW kit root
REM   MINGW_DIR     MinGW toolchain root
REM ============================================================================
setlocal
if "%QT_MINGW_DIR%"=="" ( echo [ERROR] QT_MINGW_DIR must point to the Qt MinGW kit root & exit /b 1 )
if "%MINGW_DIR%"=="" ( echo [ERROR] MINGW_DIR must point to the MinGW toolchain root & exit /b 1 )
set "ROOT=%~dp0.."
set "BUILD=%ROOT%\build\mingw"

set "PATH=%QT_MINGW_DIR%\bin;%MINGW_DIR%\bin;%PATH%"

if not exist "%BUILD%" mkdir "%BUILD%"
cd /d "%BUILD%" || exit /b 1

qmake -spec win32-g++ "%ROOT%\RobotKinematics.pro" || ( echo [ERROR] qmake failed & exit /b 1 )
mingw32-make || ( echo [ERROR] mingw32-make failed & exit /b 1 )

echo [OK] MinGW build complete.
exit /b 0
