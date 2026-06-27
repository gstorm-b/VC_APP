@echo off
REM ============================================================================
REM Run the MSVC-built RobotKinematics Qt Test executable.
REM
REM This does not build. Run build_msvc.bat first when source changed.
REM The test executable is launched from the repository root so preset paths such
REM as presets/Nachi/MZ04/nachi_mz04d.json resolve consistently.
REM ============================================================================
setlocal
if "%QT_MSVC_DIR%"=="" ( echo [ERROR] QT_MSVC_DIR must point to the Qt MSVC kit root & exit /b 1 )
set "ROOT=%~dp0.."
set "BUILD=%ROOT%\build\msvc"
set "PATH=%QT_MSVC_DIR%\bin;%PATH%"

cd /d "%ROOT%" || exit /b 1
if exist "%BUILD%\tests\RobotKinematicsTests.exe" (
    "%BUILD%\tests\RobotKinematicsTests.exe"
    exit /b %errorlevel%
)
if exist "%BUILD%\tests\release\RobotKinematicsTests.exe" (
    "%BUILD%\tests\release\RobotKinematicsTests.exe"
    exit /b %errorlevel%
)
echo [ERROR] MSVC test executable not found. Run scripts\build_msvc.bat first.
exit /b 1
