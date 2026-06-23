@echo off
REM ============================================================================
REM Clean MinGW rebuild + test for RobotKinematics.
REM
REM Use this for release verification or when switching toolchains. For normal
REM development, prefer build_mingw.bat followed by test_mingw.bat.
REM ============================================================================
setlocal
set "ROOT=%~dp0.."
set "BUILD=%ROOT%\build\mingw"

if exist "%ROOT%\.qmake.stash" del /q "%ROOT%\.qmake.stash" >nul 2>&1
if exist "%BUILD%" rmdir /s /q "%BUILD%" || exit /b 1

call "%~dp0build_mingw.bat" || exit /b %errorlevel%
call "%~dp0test_mingw.bat" || exit /b %errorlevel%
exit /b 0
