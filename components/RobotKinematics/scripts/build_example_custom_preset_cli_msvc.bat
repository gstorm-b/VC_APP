@echo off
REM ============================================================================
REM Build the no-UI custom preset CLI example with MSVC.
REM
REM Optional:
REM   QT_MSVC_DIR   Qt MSVC kit root
REM   VCVARS        full path to vcvars64.bat
REM ============================================================================
setlocal

if "%QT_MSVC_DIR%"=="" ( echo [ERROR] QT_MSVC_DIR must point to the Qt MSVC kit root & exit /b 1 )
if "%VCVARS%"=="" ( echo [ERROR] VCVARS must point to vcvars64.bat & exit /b 1 )

set "ROOT=%~dp0.."
set "PROJECT=%ROOT%\examples\CustomPresetCli\CustomPresetCli.pro"
set "BUILD=%ROOT%\examples\CustomPresetCli\build\msvc"

call "%ROOT%\scripts\build_msvc.bat"
if errorlevel 1 (
    echo [ERROR] Core RobotKinematics build failed.
    exit /b 1
)

call "%VCVARS%"
if not defined VCToolsInstallDir ( echo [ERROR] vcvars64 did not initialize the MSVC toolchain & exit /b 1 )
if not defined INCLUDE ( echo [ERROR] INCLUDE not set - vcvars64 setup incomplete & exit /b 1 )

set "PATH=%QT_MSVC_DIR%\bin;%PATH%"

if not exist "%BUILD%" mkdir "%BUILD%"
cd /d "%BUILD%" || exit /b 1

qmake "%PROJECT%" || ( echo [ERROR] qmake failed & exit /b 1 )
nmake /nologo || ( echo [ERROR] nmake failed & exit /b 1 )

echo [OK] CustomPresetCli example build complete.
exit /b 0
