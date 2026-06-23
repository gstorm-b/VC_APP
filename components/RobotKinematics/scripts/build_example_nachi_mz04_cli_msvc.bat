@echo off
REM ============================================================================
REM Build the no-UI Nachi MZ04 CLI example with MSVC.
REM
REM Optional:
REM   QT_MSVC_DIR   default C:\Qt\6.8.3\msvc2022_64
REM   VCVARS        default VS 2022 Community vcvars64.bat
REM ============================================================================
setlocal

if "%QT_MSVC_DIR%"=="" set "QT_MSVC_DIR=C:\Qt\6.8.3\msvc2022_64"
if "%VCVARS%"=="" set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

set "ROOT=%~dp0.."
set "PROJECT=%ROOT%\examples\NachiMZ04Cli\NachiMZ04Cli.pro"
set "BUILD=%ROOT%\examples\NachiMZ04Cli\build\msvc"

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

echo [OK] NachiMZ04Cli example build complete.
exit /b 0
