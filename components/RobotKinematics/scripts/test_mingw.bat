@echo off
REM ============================================================================
REM Run the MinGW-built RobotKinematics Qt Test executable.
REM
REM This does not build. Run build_mingw.bat first when source changed.
REM The test executable is launched from the repository root so preset paths such
REM as presets/Nachi/MZ04/nachi_mz04d.json resolve consistently.
REM ============================================================================
setlocal
if "%QT_MINGW_DIR%"=="" set "QT_MINGW_DIR=C:\Qt\6.8.2\mingw_64"
if "%MINGW_DIR%"=="" set "MINGW_DIR=C:\Qt\Tools\mingw1310_64"
set "ROOT=%~dp0.."
set "BUILD=%ROOT%\build\mingw"
set "PATH=%QT_MINGW_DIR%\bin;%MINGW_DIR%\bin;%PATH%"

cd /d "%ROOT%" || exit /b 1
if exist "%BUILD%\tests\RobotKinematicsTests.exe" (
    "%BUILD%\tests\RobotKinematicsTests.exe"
    exit /b %errorlevel%
)
if exist "%BUILD%\tests\release\RobotKinematicsTests.exe" (
    "%BUILD%\tests\release\RobotKinematicsTests.exe"
    exit /b %errorlevel%
)
echo [ERROR] MinGW test executable not found. Run scripts\build_mingw.bat first.
exit /b 1
