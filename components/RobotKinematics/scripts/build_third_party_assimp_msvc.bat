@echo off
setlocal

if "%QT_CMAKE%"=="" set "QT_CMAKE=C:\Qt\Tools\CMake_64\bin\cmake.exe"
if "%VCVARS%"=="" set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

set "ROOT=%~dp0.."
set "SRC=%ROOT%\third_party\assimp"
set "BUILD=%ROOT%\third_party\build\assimp"
set "INSTALL=%ROOT%\third_party\install\assimp"

if not exist "%SRC%\CMakeLists.txt" (
    echo [ERROR] Assimp source was not found at %SRC%
    exit /b 1
)

call "%VCVARS%"
if errorlevel 1 exit /b 1

if not exist "%BUILD%" mkdir "%BUILD%"

"%QT_CMAKE%" -S "%SRC%" -B "%BUILD%" -G "Ninja" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX="%INSTALL%" ^
    -DBUILD_SHARED_LIBS=ON ^
    -DASSIMP_BUILD_TESTS=OFF ^
    -DASSIMP_BUILD_ASSIMP_TOOLS=OFF ^
    -DASSIMP_BUILD_SAMPLES=OFF ^
    -DASSIMP_WARNINGS_AS_ERRORS=OFF
if errorlevel 1 exit /b 1

"%QT_CMAKE%" --build "%BUILD%"
if errorlevel 1 exit /b 1

"%QT_CMAKE%" --install "%BUILD%"
if errorlevel 1 exit /b 1

echo [OK] Assimp built and installed to %INSTALL%
exit /b 0
