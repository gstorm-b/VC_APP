# Build And Verification Guide

**Last updated:** 2026-06-24

## Rule Of Thumb

This repository is a qmake project. Always generate the Makefile with `qmake`
from the build directory before compiling. `qmake` is the project generator;
the actual compiler driver is the make tool selected by the Qt kit.

For MSVC command-line verification from Codex, use the same pattern as
`components/RobotKinematics/scripts/build_msvc*.bat`:

1. Call `vcvars64.bat` directly. Do **not** redirect its output; the component
   scripts explicitly warn that redirecting can break SDK setup.
2. Verify `VCToolsInstallDir` and `INCLUDE` are set.
3. Put the Qt kit `bin` directory and required runtime DLL directories on PATH.
4. Run `qmake` in the build directory.
5. Run `nmake /nologo` as the reliable CLI fallback.

Qt Creator can use `jom` for MSVC builds. During Phase 1 verification from the
Codex shell, `jom` did not inherit the PATH modified inside the `cmd` wrapper and
failed to find `cl`, even though `where cl` succeeded immediately before
launching `jom`. Use `jom` when launching through Qt Creator's configured kit;
otherwise prefer the `nmake` fallback below.

## MSVC App Build

```bat
set "QT_MSVC_DIR=C:\Qt\6.8.2\msvc2022_64"
set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
set "ROOT=D:\Project\ncr_picking"
set "BUILD=%ROOT%\build\phase1_ncr_picking_20260623_debug"

call "%VCVARS%"
if not defined VCToolsInstallDir exit /b 1
if not defined INCLUDE exit /b 1

set "PATH=%QT_MSVC_DIR%\bin;%BUILD%\debug;C:\opencv\build\x64\vc16\bin;C:\Program Files\Basler\pylon\Runtime\x64;%PATH%"

if not exist "%BUILD%" mkdir "%BUILD%"
cd /d "%BUILD%" || exit /b 1

qmake -o Makefile "%ROOT%\ncr_picking.pro" -spec win32-msvc CONFIG+=debug || exit /b 1
nmake /nologo || exit /b 1
```

## Architecture Contract Test

The architecture contract target contains an inline `main.moc`, so run the moc
target before the normal build when using command-line qmake/nmake:

```bat
set "QT_MSVC_DIR=C:\Qt\6.8.2\msvc2022_64"
set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
set "ROOT=D:\Project\ncr_picking"
set "BUILD=%ROOT%\build\phase1_architecture_contract_20260623_debug"

call "%VCVARS%"
if not defined VCToolsInstallDir exit /b 1
if not defined INCLUDE exit /b 1

set "PATH=%QT_MSVC_DIR%\bin;%BUILD%\debug;C:\opencv\build\x64\vc16\bin;C:\Program Files\Basler\pylon\Runtime\x64;%PATH%"

if not exist "%BUILD%" mkdir "%BUILD%"
cd /d "%BUILD%" || exit /b 1

qmake -o Makefile "%ROOT%\tests\architecture_contract_test\architecture_contract_test.pro" -spec win32-msvc CONFIG+=debug || exit /b 1
nmake /nologo -f Makefile.Debug compiler_moc_source_make_all || exit /b 1
nmake /nologo || exit /b 1
```

Run the test executable with Qt, OpenCV, Basler, the build output directory, and
Debug CRT on PATH:

```powershell
$env:QT_QPA_PLATFORM = 'minimal'
$env:Path = 'D:\Project\ncr_picking\build\phase1_architecture_contract_20260623_debug\debug;C:\Qt\6.8.2\msvc2022_64\bin;C:\opencv\build\x64\vc16\bin;C:\Program Files\Basler\pylon\Runtime\x64;C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.42.34433\debug_nonredist\x64\Microsoft.VC143.DebugCRT;C:\Windows\System32;' + $env:Path
Set-Location 'D:\Project\ncr_picking\build\phase1_architecture_contract_20260623_debug\debug'
.\architecture_contract_test.exe -silent
```

Expected success signal: process exit code `0`.
